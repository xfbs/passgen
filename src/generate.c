#include "passgen/generate.h"

#include "passgen/assert.h"
#include "passgen/container/hashmap.h"
#include "passgen/container/stack.h"
#include "passgen/markov.h"
#include "passgen/pattern/group.h"
#include "passgen/pattern/literal.h"
#include "passgen/pattern/pattern.h"
#include "passgen/pattern/range.h"
#include "passgen/pattern/repeat.h"
#include "passgen/pattern/segment.h"
#include "passgen/pattern/segment_item.h"
#include "passgen/pattern/set.h"
#include "passgen/pattern/special.h"
#include "passgen/wordlist.h"
#include "try.h"

#include <string.h>
#include <utf8proc.h>

typedef struct {
    passgen_env *env;
    size_t depth;
    void *data;
    double *entropy;
    passgen_generate_cb *func;
} passgen_generate_context;

static size_t passgen_generate_repeat(
    passgen_generate_context *context,
    const passgen_pattern_repeat *repeat);

static int passgen_generate_set(
    passgen_generate_context *context,
    const passgen_pattern_set *set);

static int passgen_generate_character(
    passgen_generate_context *context,
    const passgen_pattern_literal *character);

static int passgen_generate_special_pronounceable(
    passgen_generate_context *context,
    const passgen_pattern_special *special);

static int passgen_generate_special_wordlist(
    passgen_generate_context *context,
    const passgen_pattern_special *special);

static int passgen_generate_special(
    passgen_generate_context *context,
    const passgen_pattern_special *special);

static int passgen_generate_group(
    passgen_generate_context *context,
    const passgen_pattern_group *group);

static int passgen_generate_item(
    passgen_generate_context *context,
    const passgen_pattern_item *item);

static int passgen_generate_segment(
    passgen_generate_context *context,
    const passgen_pattern_segment *segment);

struct fillpos {
    uint32_t *buffer;
    size_t cur;
    size_t len;
};

struct fillpos_utf8 {
    uint8_t *buffer;
    size_t cur;
    size_t len;
};

static passgen_env passgen_env_default = {
    .find_entropy = false,
    .random = NULL,
};

static int passgen_generate_write_buffer(void *data, uint32_t codepoint) {
    struct fillpos *fillpos = data;

    if(fillpos->cur == fillpos->len) {
        return -1;
    }

    fillpos->buffer[fillpos->cur] = codepoint;
    fillpos->cur++;

    return 0;
}

static int passgen_generate_write_buffer_utf8(void *data, uint32_t codepoint) {
    struct fillpos_utf8 *fillpos = data;

    if(fillpos->cur == fillpos->len) {
        return -1;
    }

    if((fillpos->cur + 4) <= fillpos->len) {
        size_t bytes = utf8proc_encode_char(
            codepoint,
            (utf8proc_uint8_t *) &fillpos->buffer[fillpos->cur]);

        if(!bytes) {
            // error happened during encoding.
            return -1;
        }

        fillpos->cur += bytes;
    } else {
        char buffer[4];
        size_t bytes =
            utf8proc_encode_char(codepoint, (utf8proc_uint8_t *) &buffer[0]);

        if(!bytes) {
            // error happened during encoding.
            return -1;
        }

        if(bytes <= (fillpos->len - fillpos->cur)) {
            memcpy(&fillpos->buffer[fillpos->cur], &buffer[0], bytes);
            fillpos->cur += bytes;
        } else {
            // error: encoded doesn't fit in buffer.
            return -1;
        }
    }

    return 0;
}

/// Write JSON-escaped UTF-8 to buffer.
static int
passgen_generate_write_buffer_json_utf8(void *data, uint32_t codepoint) {
    struct fillpos_utf8 *fillpos = data;

    if(fillpos->cur == fillpos->len) {
        return -1;
    }

    unsigned char buffer[4] = {'\\', 0};
    size_t bytes = 2;
    switch(codepoint) {
        case '"':
            buffer[1] = '"';
            break;
        case '\\':
            buffer[1] = '\\';
            break;
        case '\b':
            buffer[1] = 'b';
            break;
        case '\f':
            buffer[1] = 'f';
            break;
        case '\r':
            buffer[1] = 'r';
            break;
        case '\n':
            buffer[1] = 'n';
            break;
        case '\t':
            buffer[1] = 't';
            break;
        default:
            bytes = utf8proc_encode_char(
                codepoint,
                (utf8proc_uint8_t *) &buffer[0]);
    }

    // check that no error happened.
    if(!bytes) {
        return -1;
    }

    // make sure it fits.
    if(bytes <= (fillpos->len - fillpos->cur)) {
        memcpy(&fillpos->buffer[fillpos->cur], &buffer[0], bytes);
        fillpos->cur += bytes;
    } else {
        return -1;
    }

    return 0;
}

size_t passgen_generate_fill_unicode(
    const passgen_pattern *pattern,
    passgen_env *env,
    uint32_t *buffer,
    size_t len) {
    struct fillpos fillpos = {
        .buffer = buffer,
        .len = len,
        .cur = 0,
    };

    try(passgen_generate(
        pattern,
        env,
        &fillpos,
        passgen_generate_write_buffer));

    return fillpos.cur;
}

size_t passgen_generate_fill_utf8(
    const passgen_pattern *pattern,
    passgen_env *env,
    uint8_t *buffer,
    size_t len) {
    struct fillpos_utf8 fillpos = {
        .buffer = buffer,
        .len = len,
        .cur = 0,
    };

    try(passgen_generate(
        pattern,
        env,
        &fillpos,
        passgen_generate_write_buffer_utf8));

    return fillpos.cur;
}

size_t passgen_generate_fill_json_utf8(
    const passgen_pattern *pattern,
    struct passgen_env *env,
    uint8_t *buffer,
    size_t len) {
    struct fillpos_utf8 fillpos = {
        .buffer = buffer,
        .len = len,
        .cur = 0,
    };

    try(passgen_generate(
        pattern,
        env,
        &fillpos,
        passgen_generate_write_buffer_json_utf8));

    return fillpos.cur;
}

static size_t
passgen_generate_repeat(
    passgen_generate_context *context,
    const passgen_pattern_repeat *repeat
) {
    size_t difference = repeat->max - repeat->min;

    // if there is no difference to pick, just return here
    if(0 == difference) {
        return repeat->min;
    }

    // get random number to choose from the range
    size_t choice = passgen_random_u64_max(context->env->random, difference + 1);

    // keep track of entropy
    if(context->env->find_entropy) {
        context->env->entropy *= difference + 1;
    }

    return repeat->min + choice;
}

static int passgen_generate_set(
    passgen_generate_context *context,
    const passgen_pattern_set *set) {
    // if this set is empty, we're done.
    if(set->items.len == 0) {
        return 0;
    }

    // compute number of possible codepoints
    // TODO: generate this on the fly or on demand?
    size_t possible = set->choices_list[set->items.len - 1];

    passgen_assert(possible != 0);

    size_t choice = passgen_random_u64_max(context->env->random, possible);

    // keep track of entropy
    if(context->env->find_entropy) {
        context->env->entropy *= possible;
    }

    // locate choice in list of choices.
    // TODO: binary search.
    size_t num;
    for(num = 0; num < set->items.len; num++) {
        if(choice < set->choices_list[num]) {
            break;
        }
    }

    passgen_assert(num != set->items.len);

    /* adjust choice to be relative offset */
    if(num) {
        choice -= set->choices_list[num - 1];
    }

    passgen_pattern_range *range = passgen_stack_get(&set->items, num);

    return context->func(context->data, range->start + choice);
}

static int passgen_generate_character(
    passgen_generate_context *context,
    const passgen_pattern_literal *character) {
    passgen_assert(character->count > 0);
    passgen_assert(character->count < 8);

    for(size_t i = 0; i < character->count; i++) {
        try(context->func(context->data, character->codepoints[i]));
    }

    return 0;
}

static int passgen_generate_special_pronounceable(
    passgen_generate_context *context,
    const passgen_pattern_special *special) {
    passgen_hashmap_entry *entry =
        passgen_hashmap_lookup(&context->env->wordlists, special->parameters);
    if(!entry) {
        return -1;
    }
    passgen_wordlist *wordlist = entry->value;
    if(!wordlist->parsed) {
        passgen_wordlist_parse(wordlist);
    }
    if(!wordlist->parsed_markov) {
        passgen_wordlist_parse_markov(wordlist);
    }
    passgen_markov *markov = &wordlist->markov;
    uint32_t word[128];
    size_t pos = markov->level;
    memset(word, 0, pos * sizeof(uint32_t));
    double *entropy = context->env->find_entropy ? &context->env->entropy : NULL;
    do {
        word[pos] = passgen_markov_generate(
            markov,
            &word[pos - markov->level],
            context->env->random,
            entropy);
        pos++;
    } while(word[pos - 1]);

    pos = markov->level;
    while(word[pos]) {
        context->func(context->data, word[pos]);
        pos++;
    }

    return 0;
}

static int passgen_generate_special_wordlist(
    passgen_generate_context *context,
    const passgen_pattern_special *special) {
    passgen_hashmap_entry *entry =
        passgen_hashmap_lookup(&context->env->wordlists, special->parameters);
    if(!entry) {
        return -1;
    }
    passgen_wordlist *wordlist = entry->value;
    if(!wordlist->parsed) {
        passgen_wordlist_parse(wordlist);
    }
    const char *word = passgen_wordlist_random(wordlist, context->env->random);
    while(*word) {
        context->func(context->data, *word);
        word++;
    }

    if(context->env->find_entropy) {
        context->env->entropy *= passgen_wordlist_count(wordlist);
    }

    return 0;
}

static int passgen_generate_special_preset(
    passgen_generate_context *context,
    const passgen_pattern_special *special) {
    (void) special;
    // TODO: implement
    return 0;
}

static int passgen_generate_special(
    passgen_generate_context *context,
    const passgen_pattern_special *special) {
    switch(special->kind) {
        case PASSGEN_PATTERN_SPECIAL_MARKOV:
            return passgen_generate_special_pronounceable(
                context,
                special);
        case PASSGEN_PATTERN_SPECIAL_WORDLIST:
            return passgen_generate_special_wordlist(context, special);
        case PASSGEN_PATTERN_SPECIAL_PRESET:
            return passgen_generate_special_preset(context, special);
        default:
            return 1;
    }
    return 0;
}

static int passgen_generate_item(
    passgen_generate_context *context,
    const passgen_pattern_item *item) {
    // if it is a maybe (has a question mark following it), decide first if we
    // want to emit it or not.
    if(item->maybe) {
        if(!passgen_random_bool(context->env->random)) {
            return 0;
        }
    }

    // compute random number of repetitions
    size_t reps = passgen_generate_repeat(context, &item->repeat);

    for(size_t i = 0; i < reps; i++) {
        switch(item->kind) {
            case PASSGEN_PATTERN_SET:
                try(passgen_generate_set(context, &item->data.set));
                break;
            case PASSGEN_PATTERN_LITERAL:
                try(passgen_generate_character(
                    context,
                    &item->data.literal));
                break;
            case PASSGEN_PATTERN_SPECIAL:
                try(passgen_generate_special(
                    context,
                    &item->data.special));
                break;
            case PASSGEN_PATTERN_GROUP:
                try(passgen_generate_group(context, &item->data.group));
                break;
            default:
                passgen_assert(false);
                break;
        }
    }

    // unreachable
    return 0;
}

static int passgen_generate_segment(
    passgen_generate_context *context,
    const passgen_pattern_segment *segment) {
    for(size_t i = 0; i < segment->items.len; i++) {
        passgen_pattern_item *item = passgen_stack_get(&segment->items, i);

        try(passgen_generate_item(context, item));
    }

    return 0;
}

static int passgen_generate_group(
    passgen_generate_context *context,
    const passgen_pattern_group *group) {
    // choose random segment from segments
    size_t choice = passgen_random_u64_max(context->env->random, group->multiplier_sum);
    size_t segment_index = 0;
    passgen_pattern_segment *segment =
        passgen_stack_get(&group->segments, segment_index);

    while(choice >= segment->multiplier) {
        choice -= segment->multiplier;
        segment_index += 1;
        segment = passgen_stack_get(&group->segments, segment_index);
    }

    // keep track of entropy
    if(context->env->find_entropy) {
        context->env->entropy *= group->multiplier_sum;
        context->env->entropy /= segment->multiplier;
    }

    return passgen_generate_segment(context, segment);
}

int passgen_generate(
    const passgen_pattern *pattern,
    passgen_env *env,
    void *data,
    passgen_generate_cb *func) {
    /* use default env if none was supplied. this should be relatively sane. */
    if(!env) {
        env = &passgen_env_default;
    }

    if(env->find_entropy) {
        env->entropy = 1;
    }

    passgen_generate_context context = {
        .env = env,
        .depth = env->depth,
        .func = func,
        .data = data,
        .entropy = &env->entropy,
    };

    return passgen_generate_group(&context, &pattern->group);
}
