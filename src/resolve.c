#include "passgen/container/hashmap.h"
#include "passgen/container/stack.h"
#include "passgen/pattern/env.h"
#include "passgen/pattern/group.h"
#include "passgen/pattern/pattern.h"
#include "passgen/pattern/segment.h"
#include "passgen/pattern/segment_item.h"
#include "passgen/pattern/special.h"
#include "passgen/util/error.h"
#include "passgen/wordlist.h"
#include "try.h"

#include <passgen/generate.h>

// Fail because of a special, which is named in the error and pointed to in
// the pattern.
static int fail_special(
    passgen_error *error,
    passgen_status kind,
    const passgen_pattern_special *special) {
    passgen_error_set_at(error, kind, special->start, special->end);
    passgen_error_detail_set(error, special->parameters);
    return kind;
}

// Fail because of a special, which was caused by another error. The cause is
// moved into the error.
static int fail_special_cause(
    passgen_error *error,
    passgen_status kind,
    const passgen_pattern_special *special,
    passgen_error *cause) {
    passgen_error_wrap(error, kind, cause);
    passgen_error_detail_set(error, special->parameters);
    if(error) {
        error->has_position = true;
        error->start = special->start;
        error->end = special->end;
    }
    return kind;
}

// Finds the wordlist which a special uses, and reads it (and makes the Markov
// chain from it, if that is what it is used for). A wordlist which is used
// more than once is only read once.
static int resolve_wordlist(
    passgen_env *env,
    passgen_error *error,
    passgen_pattern_special *special,
    bool markov) {
    special->wordlist = NULL;

    passgen_hashmap_entry *entry =
        passgen_hashmap_lookup(&env->wordlists, special->parameters);
    if(!entry) {
        return fail_special(error, PASSGEN_ERROR_WORDLIST_NOT_FOUND, special);
    }
    passgen_wordlist *wordlist = entry->value;

    passgen_error cause;
    passgen_error_init(&cause);
    passgen_status status = PASSGEN_OK;

    if(!wordlist->parsed) {
        status = passgen_wordlist_parse(wordlist, &cause);
    }

    if(status == PASSGEN_OK && markov && !wordlist->parsed_markov) {
        status = passgen_wordlist_parse_markov(wordlist, &cause);
    }

    if(status != PASSGEN_OK) {
        return fail_special_cause(
            error,
            PASSGEN_ERROR_WORDLIST_READ,
            special,
            &cause);
    }

    // nothing can be picked from an empty wordlist, and an empty Markov chain
    // cannot generate anything.
    size_t count =
        markov ? wordlist->markov.count : passgen_wordlist_count(wordlist);
    if(count == 0) {
        return fail_special(error, PASSGEN_ERROR_WORDLIST_EMPTY, special);
    }

    special->wordlist = wordlist;
    return PASSGEN_OK;
}

static int resolve_special(
    passgen_env *env,
    passgen_error *error,
    passgen_pattern_special *special) {
    // the parser does not let through a special without a name (as in
    // `\w{}`), but a pattern which was built by hand may have one.
    if(!special->parameters) {
        return passgen_error_set(error, PASSGEN_ERROR_INTERNAL);
    }

    switch(special->kind) {
        case PASSGEN_PATTERN_SPECIAL_WORDLIST:
            return resolve_wordlist(env, error, special, false);
        case PASSGEN_PATTERN_SPECIAL_MARKOV:
            return resolve_wordlist(env, error, special, true);
        case PASSGEN_PATTERN_SPECIAL_PRESET:
            return fail_special(error, PASSGEN_ERROR_PRESET, special);
    }

    return passgen_error_set(error, PASSGEN_ERROR_INTERNAL);
}

static int resolve_group(
    passgen_env *env,
    passgen_error *error,
    passgen_pattern_group *group);

static int resolve_item(
    passgen_env *env,
    passgen_error *error,
    passgen_pattern_item *item) {
    switch(item->kind) {
        case PASSGEN_PATTERN_SET:
        case PASSGEN_PATTERN_LITERAL:
            return PASSGEN_OK;
        case PASSGEN_PATTERN_SPECIAL:
            return resolve_special(env, error, &item->data.special);
        case PASSGEN_PATTERN_GROUP:
            return resolve_group(env, error, &item->data.group);
    }

    return passgen_error_set(error, PASSGEN_ERROR_INTERNAL);
}

// this recurses as deeply as groups are nested, which the parser limits.
static int resolve_group(
    passgen_env *env,
    passgen_error *error,
    passgen_pattern_group *group) {
    for(size_t i = 0; i < group->segments.len; i++) {
        passgen_pattern_segment *segment =
            passgen_stack_get(&group->segments, i);
        for(size_t j = 0; j < segment->items.len; j++) {
            try(resolve_item(env, error, passgen_stack_get(&segment->items, j)));
        }
    }

    return PASSGEN_OK;
}

passgen_status
passgen_resolve(passgen_pattern *pattern, passgen_env *env, passgen_error *error) {
    // whatever happens, the pattern is only resolved if all of it is.
    pattern->env = NULL;

    passgen_status status =
        (passgen_status) resolve_group(env, error, &pattern->group);
    if(status != PASSGEN_OK) {
        return status;
    }

    pattern->env = env;
    pattern->env_version = env->version;
    return PASSGEN_OK;
}
