#include "passgen/generate.h"

#include "passgen/assert.h"
#include "passgen/container/stack.h"
#include "passgen/markov.h"
#include "passgen/pattern/env.h"
#include "passgen/pattern/group.h"
#include "passgen/pattern/literal.h"
#include "passgen/pattern/pattern.h"
#include "passgen/pattern/preset.h"
#include "passgen/pattern/range.h"
#include "passgen/pattern/repeat.h"
#include "passgen/pattern/segment.h"
#include "passgen/pattern/segment_item.h"
#include "passgen/pattern/set.h"
#include "passgen/pattern/special.h"
#include "passgen/util/error.h"
#include "passgen/util/utf8.h"
#include "passgen/wordlist.h"
#include "try.h"

#include <math.h>
#include <string.h>

typedef struct {
    passgen_env *env;
    // how many more steps may be taken.
    size_t steps;
    double *entropy;
    passgen_error *error;
    void *data;
    passgen_generate_cb *func;
} passgen_generate_context;

// Fail, for the given reason. Everything below returns a passgen_status as an
// int, so that it can be passed up with try().
static inline int fail(passgen_generate_context *context, passgen_status kind) {
    return passgen_error_set(context->error, kind);
}

// Fail because of something to do with a wordlist, which was caused by
// another error. The cause is moved into the error.
static inline int fail_wordlist_cause(
    passgen_generate_context *context,
    passgen_status kind,
    const char *name,
    passgen_error *cause) {
    passgen_error_wrap(context->error, kind, cause);
    passgen_error_detail_set(context->error, name);
    return kind;
}

// Emit a character
static inline int emit(passgen_generate_context *context, uint32_t codepoint) {
    // if the source of randomness has failed, this is not what should have
    // been generated.
    if(passgen_random_failed(context->env->random)) {
        return fail(context, PASSGEN_ERROR_RANDOM);
    }

    if(0 != context->func(context->data, codepoint)) {
        return fail(context, PASSGEN_ERROR_ABORTED);
    }

    return PASSGEN_OK;
}

// Take a step, which is generating one item once. Every step does an amount
// of work which is bounded by the size of the pattern (or of a word), so this
// bounds how long generating takes.
static inline int step(passgen_generate_context *context) {
    if(!context->steps) {
        return fail(context, PASSGEN_ERROR_STEP_LIMIT);
    }

    context->steps -= 1;
    return 0;
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

// the callbacks which fill a buffer can only say that generating should stop,
// so they remember why here.
struct fillpos {
    uint32_t *buffer;
    size_t cur;
    size_t len;
    passgen_status status;
};

struct fillpos_utf8 {
    uint8_t *buffer;
    size_t cur;
    size_t len;
    passgen_status status;
};

static int passgen_generate_write_buffer(void *data, uint32_t codepoint) {
    struct fillpos *fillpos = data;

    if(fillpos->cur == fillpos->len) {
        fillpos->status = PASSGEN_ERROR_BUFFER_TOO_SMALL;
        return -1;
    }

    fillpos->buffer[fillpos->cur] = codepoint;
    fillpos->cur++;

    return 0;
}

static int passgen_generate_write_buffer_utf8(void *data, uint32_t codepoint) {
    struct fillpos_utf8 *fillpos = data;

    if(fillpos->cur == fillpos->len) {
        fillpos->status = PASSGEN_ERROR_BUFFER_TOO_SMALL;
        return -1;
    }

    if((fillpos->cur + 4) <= fillpos->len) {
        int bytes = passgen_utf8_encode_codepoint(
            (uint8_t *) &fillpos->buffer[fillpos->cur],
            codepoint);

        if(bytes < 0) {
            // error happened during encoding.
            fillpos->status = PASSGEN_ERROR_INTERNAL;
            return -1;
        }

        fillpos->cur += bytes;
    } else {
        char buffer[4];
        int bytes =
            passgen_utf8_encode_codepoint((uint8_t *) &buffer[0], codepoint);

        if(bytes < 0) {
            // error happened during encoding.
            fillpos->status = PASSGEN_ERROR_INTERNAL;
            return -1;
        }

        if((size_t)bytes <= (fillpos->len - fillpos->cur)) {
            memcpy(&fillpos->buffer[fillpos->cur], &buffer[0], bytes);
            fillpos->cur += bytes;
        } else {
            // error: encoded doesn't fit in buffer.
            fillpos->status = PASSGEN_ERROR_BUFFER_TOO_SMALL;
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
        fillpos->status = PASSGEN_ERROR_BUFFER_TOO_SMALL;
        return -1;
    }

    unsigned char buffer[6] = {'\\', 0};
    int bytes = 2;
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
            if(codepoint < 0x20) {
                // remaining control characters must be escaped in JSON.
                static const char hex[] = "0123456789abcdef";
                buffer[1] = 'u';
                buffer[2] = '0';
                buffer[3] = '0';
                buffer[4] = hex[codepoint >> 4];
                buffer[5] = hex[codepoint & 15];
                bytes = 6;
            } else {
                bytes = passgen_utf8_encode_codepoint(
                    (uint8_t *) &buffer[0],
                    codepoint);
            }
    }

    // check that no error happened.
    if(bytes <= 0) {
        fillpos->status = PASSGEN_ERROR_INTERNAL;
        return -1;
    }

    // make sure it fits.
    if((size_t) bytes <= (fillpos->len - fillpos->cur)) {
        memcpy(&fillpos->buffer[fillpos->cur], &buffer[0], bytes);
        fillpos->cur += bytes;
    } else {
        fillpos->status = PASSGEN_ERROR_BUFFER_TOO_SMALL;
        return -1;
    }

    return 0;
}

// Runs a callback which fills a buffer, and works out the status from what
// it has to say about why it stopped.
#define PASSGEN_GENERATE_FILL(fillpos, callback)                             \
    do {                                                                     \
        passgen_status status = passgen_generate(                            \
            pattern,                                                         \
            env,                                                             \
            error,                                                           \
            entropy,                                                         \
            &fillpos,                                                        \
            callback);                                                       \
                                                                             \
        /* if the callback is what stopped it, it knows the reason. */       \
        if(status == PASSGEN_ERROR_ABORTED && fillpos.status != PASSGEN_OK) { \
            status = passgen_error_set(error, fillpos.status);               \
        }                                                                    \
                                                                             \
        if(written) {                                                        \
            *written = (status == PASSGEN_OK) ? fillpos.cur : 0;             \
        }                                                                    \
                                                                             \
        return status;                                                       \
    } while(0)

passgen_status passgen_generate_fill_unicode(
    const passgen_pattern *pattern,
    passgen_env *env,
    passgen_error *error,
    double *entropy,
    uint32_t *buffer,
    size_t len,
    size_t *written) {
    struct fillpos fillpos = {
        .buffer = buffer,
        .len = len,
        .cur = 0,
        .status = PASSGEN_OK,
    };

    PASSGEN_GENERATE_FILL(fillpos, passgen_generate_write_buffer);
}

passgen_status passgen_generate_fill_utf8(
    const passgen_pattern *pattern,
    passgen_env *env,
    passgen_error *error,
    double *entropy,
    uint8_t *buffer,
    size_t len,
    size_t *written) {
    struct fillpos_utf8 fillpos = {
        .buffer = buffer,
        .len = len,
        .cur = 0,
        .status = PASSGEN_OK,
    };

    PASSGEN_GENERATE_FILL(fillpos, passgen_generate_write_buffer_utf8);
}

passgen_status passgen_generate_fill_json_utf8(
    const passgen_pattern *pattern,
    passgen_env *env,
    passgen_error *error,
    double *entropy,
    uint8_t *buffer,
    size_t len,
    size_t *written) {
    struct fillpos_utf8 fillpos = {
        .buffer = buffer,
        .len = len,
        .cur = 0,
        .status = PASSGEN_OK,
    };

    PASSGEN_GENERATE_FILL(fillpos, passgen_generate_write_buffer_json_utf8);
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
        *context->entropy += log2((double) difference + 1);
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

    passgen_assert(set->choices_list != NULL);

    // compute number of possible choices
    size_t possible = set->choices_list[set->items.len - 1];
    passgen_assert(possible != 0);

    // a set of a single character (such as `[a]` or `[aa]`, which are merged
    // into one range) has nothing to choose from, so it takes no randomness,
    // like a repetition with a fixed count. the first choice is that
    // character.
    passgen_pattern_range *first = passgen_stack_get(&set->items, 0);
    bool single = set->items.len == 1 && first->start == first->end;
    size_t choice =
        single ? 0 : passgen_random_u64_max(context->env->random, possible);

    // locate the range of the choice: the first one whose cumulative number
    // of choices is larger than it.
    size_t low = 0;
    size_t high = set->items.len - 1;
    while(low < high) {
        size_t middle = low + (high - low) / 2;
        if(choice < set->choices_list[middle]) {
            high = middle;
        } else {
            low = middle + 1;
        }
    }

    // adjust choice to be relative offset
    if(low) {
        choice -= set->choices_list[low - 1];
    }

    passgen_pattern_range *range = passgen_stack_get(&set->items, low);
    passgen_assert(range->multiplier > 0);

    // keep track of entropy: the set had possible equally likely choices, of
    // which multiplier many result in this character.
    if(context->entropy) {
        *context->entropy +=
            log2((double) possible) - log2((double) range->multiplier);
    }

    // every character of the range is there multiplier many times.
    return emit(context, range->start + (uint32_t) (choice / range->multiplier));
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
    // resolving made sure that the Markov chain is there and not empty.
    passgen_markov *markov = &special->wordlist->markov;

    // sliding window of the previous `level` codepoints (initially all zero,
    // which marks the start of a word), plus one slot for the next codepoint.
    // generated words have no upper bound on their length, so they are emitted
    // as they are generated rather than collected in a fixed-size buffer.
    size_t level = markov->level;
    uint32_t window[UINT8_MAX + 1] = {0};
    while(true) {
        uint32_t next = passgen_markov_generate(
            markov,
            window,
            context->env->random,
            context->entropy);
        if(!next) {
            break;
        }
        try(emit(context, next));
        memmove(&window[0], &window[1], (level - 1) * sizeof(window[0]));
        window[level - 1] = next;
    }

    return 0;
}

static int passgen_generate_special_wordlist(
    passgen_generate_context *context,
    const passgen_pattern_special *special) {
    // resolving made sure that the wordlist is read and not empty.
    passgen_wordlist *wordlist = special->wordlist;
    const char *name = special->parameters;

    // pick word at random
    const char *word = passgen_wordlist_random(wordlist, context->env->random);

    // UTF8-decode word and write codepoints, in chunks so that words of any
    // length work.
    const uint8_t *word_pos = (const uint8_t *) word;
    const uint8_t *word_end = word_pos + strlen(word);
    uint32_t codepoints[128];
    int decode_ret;
    do {
        uint32_t *codepoints_pos = &codepoints[0];
        decode_ret = passgen_utf8_decode(
            &codepoints_pos,
            128,
            NULL,
            &word_pos,
            word_end - word_pos);
        if(decode_ret < PASSGEN_UTF8_SUCCESS) {
            // the word which was picked is not valid UTF-8.
            passgen_error cause;
            passgen_error_init(&cause);
            passgen_error_set(&cause, PASSGEN_ERROR_UTF8);
            return fail_wordlist_cause(
                context,
                PASSGEN_ERROR_WORDLIST_READ,
                name,
                &cause);
        }
        for(size_t i = 0; &codepoints[i] < codepoints_pos; i++) {
            try(emit(context, codepoints[i]));
        }
    } while(decode_ret > PASSGEN_UTF8_SUCCESS);

    if(context->entropy) {
        *context->entropy += log2((double) passgen_wordlist_count(wordlist));
    }

    return 0;
}

static int passgen_generate_special(
    passgen_generate_context *context,
    const passgen_pattern_special *special) {
    // resolving fails for wordlists and presets which it cannot find.
    if(!special->wordlist) {
        return fail(context, PASSGEN_ERROR_INTERNAL);
    }

    switch(special->kind) {
        case PASSGEN_PATTERN_SPECIAL_MARKOV:
            return passgen_generate_special_markov(context, special);
        case PASSGEN_PATTERN_SPECIAL_WORDLIST:
            return passgen_generate_special_wordlist(context, special);
        case PASSGEN_PATTERN_SPECIAL_PRESET:
            // a preset is generated like a group, which makes \p{name} the
            // same as the pattern of the preset in parentheses, and so the
            // same as the pattern on its own (as with --preset).
            return passgen_generate_group(
                context,
                &special->preset->pattern.group);
        default:
            return fail(context, PASSGEN_ERROR_INTERNAL);
    }
}

static int passgen_generate_item(
    passgen_generate_context *context,
    const passgen_pattern_item *item) {
    // if it is a maybe (has a question mark following it), decide first if we
    // want to emit it or not.
    if(item->maybe) {
        // either way, this is a choice between two options.
        if(context->entropy) {
            *context->entropy += 1;
        }

        if(!passgen_random_bool(context->env->random)) {
            return 0;
        }
    }

    // compute random number of repetitions
    size_t reps = passgen_generate_repeat(context, &item->repeat);

    for(size_t i = 0; i < reps; i++) {
        try(step(context));

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
                // the parser does not make any other kinds of items.
                return fail(context, PASSGEN_ERROR_INTERNAL);
        }
    }

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
    // every segment of the group has a multiplier of zero.
    if(group->multiplier_sum == 0) {
        return fail(context, PASSGEN_ERROR_GROUP_EMPTY);
    }

    // choose random segment from segments. a group with a single segment
    // (such as `(abc)` or `({2}abc)`, and every pattern itself) has nothing to
    // choose from, so it takes no randomness. this makes `(abc)` the same as
    // `abc`, and a preset the same as its pattern.
    size_t choice =
        group->segments.len == 1
            ? 0
            : passgen_random_u64_max(context->env->random, group->multiplier_sum);
    size_t segment_index = 0;
    passgen_pattern_segment *segment =
        passgen_stack_get(&group->segments, segment_index);

    while(segment && choice >= segment->multiplier) {
        choice -= segment->multiplier;
        segment_index += 1;
        segment = passgen_stack_get(&group->segments, segment_index);
    }

    // this only happens if the sum of the multipliers is not what the
    // multipliers of the segments add up to, which would be a bug in the
    // parser.
    if(!segment) {
        return fail(context, PASSGEN_ERROR_INTERNAL);
    }

    // keep track of entropy
    // the segment was one of multiplier_sum equally likely choices, of which
    // multiplier many result in this segment.
    if(context->entropy) {
        *context->entropy += log2((double) group->multiplier_sum) -
                             log2((double) segment->multiplier);
    }

    return passgen_generate_segment(context, segment);
}

passgen_status passgen_generate(
    const passgen_pattern *pattern,
    passgen_env *env,
    passgen_error *error,
    double *entropy,
    void *data,
    passgen_generate_cb *func) {
    // when entropy collection is request (by passing a non-NULL pointer),
    // initialize it.
    if(entropy) {
        *entropy = 0.0;
    }

    // the pattern points to wordlists of the environment it was resolved
    // with, which may have been replaced since.
    if(pattern->env != env || pattern->env_version != env->version) {
        return passgen_error_set(error, PASSGEN_ERROR_INVALID_ARGUMENT);
    }

    passgen_generate_context context = {
        .env = env,
        .steps = env->step_limit,
        .func = func,
        .data = data,
        .entropy = entropy,
        .error = error,
    };

    passgen_status status =
        (passgen_status) passgen_generate_group(&context, &pattern->group);

    // choices which did not result in output (such as leaving something
    // out) may have been made with a source which has failed, too.
    if(status == PASSGEN_OK && passgen_random_failed(env->random)) {
        status = fail(&context, PASSGEN_ERROR_RANDOM);
    }

    return status;
}
