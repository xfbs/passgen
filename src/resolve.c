#include "passgen/container/hashmap.h"
#include "passgen/container/stack.h"
#include "passgen/parser/parser.h"
#include "passgen/pattern/env.h"
#include "passgen/pattern/group.h"
#include "passgen/pattern/pattern.h"
#include "passgen/pattern/preset.h"
#include "passgen/pattern/segment.h"
#include "passgen/pattern/segment_item.h"
#include "passgen/pattern/special.h"
#include "passgen/util/error.h"
#include "passgen/wordlist.h"
#include "try.h"

#include <passgen/generate.h>

// How many levels groups may be nested in all, through presets: the pattern
// itself, and as many groups in it as the parser allows. Generating recurses
// this deeply.
#define DEPTH_LIMIT (PASSGEN_PARSER_DEPTH_LIMIT + 1)

typedef struct {
    passgen_env *env;
    passgen_error *error;
} resolve_context;

// Fail because of a special, which is named in the error and pointed to in
// the pattern.
static int fail_special(
    resolve_context *context,
    passgen_status kind,
    const passgen_pattern_special *special) {
    passgen_error_set_at(context->error, kind, special->start, special->end);
    passgen_error_detail_set(context->error, special->parameters);
    return kind;
}

// Fail because of a special, which was caused by another error. The cause is
// moved into the error.
static int fail_special_cause(
    resolve_context *context,
    passgen_status kind,
    const passgen_pattern_special *special,
    passgen_error *cause) {
    passgen_error *error = context->error;
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
    resolve_context *context,
    passgen_pattern_special *special,
    bool markov) {
    special->wordlist = NULL;

    passgen_hashmap_entry *entry =
        passgen_hashmap_lookup(&context->env->wordlists, special->parameters);
    if(!entry) {
        return fail_special(context, PASSGEN_ERROR_WORDLIST_NOT_FOUND, special);
    }
    passgen_wordlist *wordlist = entry->value;

    passgen_error cause;
    passgen_error_init(&cause);
    passgen_status status = PASSGEN_OK;

    if(!wordlist->parsed) {
        status = passgen_wordlist_parse(wordlist, &cause);
    }

    if(status == PASSGEN_OK && markov) {
        status = passgen_wordlist_parse_markov(
            wordlist,
            special->markov_length,
            &cause);
    }

    if(status != PASSGEN_OK) {
        return fail_special_cause(
            context,
            PASSGEN_ERROR_WORDLIST_READ,
            special,
            &cause);
    }

    // nothing can be picked from an empty wordlist, and an empty Markov chain
    // cannot generate anything.
    size_t count =
        markov
            ? passgen_wordlist_markov(wordlist, special->markov_length)->count
            : passgen_wordlist_count(wordlist);
    if(count == 0) {
        return fail_special(context, PASSGEN_ERROR_WORDLIST_EMPTY, special);
    }

    special->wordlist = wordlist;
    return PASSGEN_OK;
}

static int resolve_group(
    resolve_context *context,
    passgen_pattern_group *group,
    size_t level,
    size_t *depth);

// Finds the preset which a special uses, parses it, and resolves it, with
// what it uses in turn. Its pattern is generated like a group, so it is
// nested one level deeper than the special, which is at the given level.
// Sets depth to how many levels it has.
static int resolve_preset(
    resolve_context *context,
    passgen_pattern_special *special,
    size_t level,
    size_t *depth) {
    special->preset = NULL;

    passgen_hashmap_entry *entry =
        passgen_hashmap_lookup(&context->env->presets, special->parameters);
    if(!entry) {
        return fail_special(context, PASSGEN_ERROR_PRESET_NOT_FOUND, special);
    }
    passgen_preset *preset = entry->value;

    // it is already being resolved, further up. it would never end.
    if(preset->resolving) {
        return fail_special(context, PASSGEN_ERROR_PRESET_CYCLE, special);
    }

    // a preset which is used more than once is only resolved once, unless the
    // environment changed since.
    bool resolved = preset->pattern.env == context->env &&
                    preset->pattern.env_version == context->env->version;

    // checked before going any deeper, so that resolving does not recurse
    // without bounds either.
    if(level + 1 > DEPTH_LIMIT ||
       (resolved && level + preset->depth > DEPTH_LIMIT)) {
        return fail_special(context, PASSGEN_ERROR_DEPTH_LIMIT, special);
    }

    if(!resolved) {
        // the problem is somewhere in the preset, which the cause points to.
        // this says where the preset is used.
        passgen_error cause;
        passgen_error_init(&cause);
        resolve_context inner = {context->env, &cause};

        passgen_status status = passgen_preset_parse(preset, &cause);
        if(status == PASSGEN_OK) {
            preset->resolving = true;
            preset->pattern.env = NULL;
            status = resolve_group(
                &inner,
                &preset->pattern.group,
                level + 1,
                &preset->depth);
            preset->resolving = false;
        }

        if(status == PASSGEN_ERROR_OUT_OF_MEMORY) {
            passgen_error_free(&cause);
            return passgen_error_set(context->error, status);
        }

        // nesting too deeply is not a problem with the preset (or the ones
        // which it uses), but with where it is used: the same as for one
        // which was resolved before. this ends up at the outermost preset.
        if(status == PASSGEN_ERROR_DEPTH_LIMIT) {
            passgen_error_free(&cause);
            return fail_special(context, PASSGEN_ERROR_DEPTH_LIMIT, special);
        }
        if(status != PASSGEN_OK) {
            return fail_special_cause(
                context,
                PASSGEN_ERROR_PRESET_INVALID,
                special,
                &cause);
        }

        preset->pattern.env = context->env;
        preset->pattern.env_version = context->env->version;
    }

    special->preset = preset;
    *depth = preset->depth;
    return PASSGEN_OK;
}

static int resolve_special(
    resolve_context *context,
    passgen_pattern_special *special,
    size_t level,
    size_t *depth) {
    // the parser does not let through a special without a name (as in
    // `\w{}`), but a pattern which was built by hand may have one.
    if(!special->parameters) {
        return passgen_error_set(context->error, PASSGEN_ERROR_INTERNAL);
    }

    switch(special->kind) {
        case PASSGEN_PATTERN_SPECIAL_WORDLIST:
            return resolve_wordlist(context, special, false);
        case PASSGEN_PATTERN_SPECIAL_MARKOV:
            return resolve_wordlist(context, special, true);
        case PASSGEN_PATTERN_SPECIAL_PRESET:
            return resolve_preset(context, special, level, depth);
    }

    return passgen_error_set(context->error, PASSGEN_ERROR_INTERNAL);
}

// Resolves an item which is at the given level, and sets depth to how many
// levels of groups it has (and the presets it uses).
static int resolve_item(
    resolve_context *context,
    passgen_pattern_item *item,
    size_t level,
    size_t *depth) {
    *depth = 0;

    switch(item->kind) {
        case PASSGEN_PATTERN_SET:
        case PASSGEN_PATTERN_LITERAL:
            return PASSGEN_OK;
        case PASSGEN_PATTERN_SPECIAL:
            return resolve_special(context, &item->data.special, level, depth);
        case PASSGEN_PATTERN_GROUP:
            return resolve_group(context, &item->data.group, level + 1, depth);
    }

    return passgen_error_set(context->error, PASSGEN_ERROR_INTERNAL);
}

// Resolves a group which is at the given level (the pattern itself is at the
// first one), and sets depth to how many levels it has, counting itself.
//
// This recurses as deeply as groups are nested, through presets, which is
// limited to DEPTH_LIMIT.
static int resolve_group(
    resolve_context *context,
    passgen_pattern_group *group,
    size_t level,
    size_t *depth) {
    // only groups in presets can be nested this deeply, which the preset it
    // is in says where it is used.
    if(level > DEPTH_LIMIT) {
        return passgen_error_set(context->error, PASSGEN_ERROR_DEPTH_LIMIT);
    }

    size_t deepest = 0;
    for(size_t i = 0; i < group->segments.len; i++) {
        passgen_pattern_segment *segment =
            passgen_stack_get(&group->segments, i);
        for(size_t j = 0; j < segment->items.len; j++) {
            size_t item_depth = 0;
            try(resolve_item(
                context,
                passgen_stack_get(&segment->items, j),
                level,
                &item_depth));
            if(item_depth > deepest) {
                deepest = item_depth;
            }
        }
    }

    *depth = deepest + 1;
    return PASSGEN_OK;
}

passgen_status passgen_resolve(
    passgen_pattern *pattern,
    passgen_env *env,
    passgen_error *error) {
    // whatever happens, the pattern is only resolved if all of it is.
    pattern->env = NULL;

    resolve_context context = {env, error};
    size_t depth = 0;
    passgen_status status =
        (passgen_status) resolve_group(&context, &pattern->group, 1, &depth);
    if(status != PASSGEN_OK) {
        return status;
    }

    pattern->env = env;
    pattern->env_version = env->version;
    return PASSGEN_OK;
}
