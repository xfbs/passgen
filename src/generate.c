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
#include "passgen/util/utf8.h"
#include "passgen/wordlist.h"
#include "try.h"

#include <string.h>

typedef struct {
    passgen_env *env;
    size_t depth;
    double *entropy;
    void *data;
    passgen_generate_cb *func;
} passgen_generate_context;

// Emit a character
static inline int emit(passgen_generate_context *context, uint32_t codepoint) {
    return context->func(context->data, codepoint);
}

// Recurse
static inline int descend(passgen_generate_context *context) {
    if(!context->depth) {
        return 1;
    }

    context->depth -= 1;
    return 0;
}

static inline void ascend(passgen_generate_context *context) {
    context->depth += 1;
}

static size_t passgen_generate_repeat(
    passgen_generate_context *context,
    const passgen_pattern_repeat *repeat);

static int passgen_generate_set(
    passgen_generate_context *context,
    const passgen_pattern_set *set);

static int passgen_generate_literal(
    passgen_generate_context *context,
    const passgen_pattern_literal *character);

static int passgen_generate_special_markov(
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
        int bytes = passgen_utf8_encode_codepoint(
            (uint8_t *) &fillpos->buffer[fillpos->cur],
            codepoint);

        if(bytes < 0) {
            // error happened during encoding.
            return -1;
        }

        fillpos->cur += bytes;
    } else {
        char buffer[4];
        int bytes =
            passgen_utf8_encode_codepoint((uint8_t *) &buffer[0], codepoint);

        if(bytes < 0) {
            // error happened during encoding.
            return -1;
        }

        if((size_t)bytes <= (fillpos->len - fillpos->cur)) {
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
            bytes = passgen_utf8_encode_codepoint(
                (uint8_t *) &buffer[0],
                codepoint);
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
    double *entropy,
    uint32_t *buffer,
    size_t len) {
    struct fillpos fillpos = {
        .buffer = buffer,
        .len = len,
        .cur = 0,
    };

    int ret = passgen_generate(
        pattern,
        env,
        entropy,
        &fillpos,
        passgen_generate_write_buffer);

    if(ret != 0) {
        return 0;
    }

    return fillpos.cur;
}

size_t passgen_generate_fill_utf8(
    const passgen_pattern *pattern,
    passgen_env *env,
    double *entropy,
    uint8_t *buffer,
    size_t len) {
    struct fillpos_utf8 fillpos = {
        .buffer = buffer,
        .len = len,
        .cur = 0,
    };

    int ret = passgen_generate(
        pattern,
        env,
        entropy,
        &fillpos,
        passgen_generate_write_buffer_utf8);

    if(ret != 0) {
        return 0;
    }

    return fillpos.cur;
}

size_t passgen_generate_fill_json_utf8(
    const passgen_pattern *pattern,
    passgen_env *env,
    double *entropy,
    uint8_t *buffer,
    size_t len) {
    struct fillpos_utf8 fillpos = {
        .buffer = buffer,
        .len = len,
        .cur = 0,
    };

    int ret = passgen_generate(
        pattern,
        env,
        entropy,
        &fillpos,
        passgen_generate_write_buffer_json_utf8);

    if(ret != 0) {
        return 0;
    }

    return fillpos.cur;
}

static size_t passgen_generate_repeat(
    passgen_generate_context *context,
    const passgen_pattern_repeat *repeat) {
    size_t difference = repeat->max - repeat->min;

    // if there is no difference to pick, just return here
    if(0 == difference) {
        return repeat->min;
    }

    // get random number to choose from the range
    size_t choice =
        passgen_random_u64_max(context->env->random, difference + 1);

    // keep track of entropy
    if(context->entropy) {
        *context->entropy *= difference + 1;
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
    size_t possible = set->choices_list[set->items.len - 1];
    passgen_assert(possible != 0);
    size_t choice = passgen_random_u64_max(context->env->random, possible);

    // keep track of entropy
    if(context->entropy) {
        *context->entropy *= possible;
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

    return emit(context, range->start + choice);
}

static int passgen_generate_literal(
    passgen_generate_context *context,
    const passgen_pattern_literal *literal) {
    passgen_assert(literal->count > 0);
    passgen_assert(literal->count < 8);

    for(size_t i = 0; i < literal->count; i++) {
        try(emit(context, literal->codepoints[i]));
    }

    return 0;
}

static int passgen_generate_special_markov(
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
    double *entropy = context->entropy ? &*context->entropy : NULL;
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
        try(emit(context, word[pos]));
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

    // pick word at random
    const char *word = passgen_wordlist_random(wordlist, context->env->random);

    // UTF8-decode word and write codepoints
    // TODO: handle longer words
    size_t word_len = strlen(word);
    const char **word_pos = &word;
    uint32_t codepoints[128];
    uint32_t *codepoints_pos = &codepoints[0];
    try(passgen_utf8_decode(&codepoints_pos, 128, NULL, (const uint8_t **) word_pos, word_len));
    for(int i = 0; &codepoints[i] < codepoints_pos; i++) {
        try(emit(context, codepoints[i]));
    }

    if(context->entropy) {
        *context->entropy *= passgen_wordlist_count(wordlist);
    }

    return 0;
}

static int passgen_generate_special_preset(
    passgen_generate_context *context,
    const passgen_pattern_special *special) {
    (void) context;
    (void) special;
    // TODO: implement
    return 0;
}

static int passgen_generate_special(
    passgen_generate_context *context,
    const passgen_pattern_special *special) {
    switch(special->kind) {
        case PASSGEN_PATTERN_SPECIAL_MARKOV:
            return passgen_generate_special_markov(context, special);
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
                try(passgen_generate_literal(context, &item->data.literal));
                break;
            case PASSGEN_PATTERN_SPECIAL:
                try(passgen_generate_special(context, &item->data.special));
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
    // descend in depth
    try(descend(context));

    if(group->multiplier_sum == 0) {
        return 1;
    }

    // choose random segment from segments
    size_t choice =
        passgen_random_u64_max(context->env->random, group->multiplier_sum);
    size_t segment_index = 0;
    passgen_pattern_segment *segment =
        passgen_stack_get(&group->segments, segment_index);

    while(choice >= segment->multiplier) {
        choice -= segment->multiplier;
        segment_index += 1;
        segment = passgen_stack_get(&group->segments, segment_index);
    }

    // keep track of entropy
    if(context->entropy) {
        *context->entropy *= group->multiplier_sum;
        *context->entropy /= segment->multiplier;
    }

    try(passgen_generate_segment(context, segment));

    ascend(context);

    return 0;
}

int passgen_generate(
    const passgen_pattern *pattern,
    passgen_env *env,
    double *entropy,
    void *data,
    passgen_generate_cb *func) {
    // when entropy collection is request (by passing a non-NULL pointer),
    // initialize it.
    if(entropy) {
        *entropy = 1.0;
    }

    passgen_generate_context context = {
        .env = env,
        .depth = env->depth_limit,
        .func = func,
        .data = data,
        .entropy = entropy,
    };

    return passgen_generate_group(&context, &pattern->group);
}
