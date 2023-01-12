#include "passgen/generate.h"

#include "passgen/assert.h"
#include "passgen/markov.h"
#include "passgen/pattern/chars.h"
#include "passgen/pattern/group.h"
#include "passgen/pattern/pattern.h"
#include "passgen/pattern/pattern_kind.h"
#include "passgen/pattern/range.h"
#include "passgen/pattern/repeat.h"
#include "passgen/pattern/segment.h"
#include "passgen/pattern/segment_item.h"
#include "passgen/pattern/set.h"
#include "passgen/pattern/special.h"
#include "passgen/pattern/special_kind.h"
#include "passgen/util/hashmap.h"
#include "passgen/util/stack.h"
#include "passgen/wordlist.h"

#include <string.h>
#include <utf8proc.h>

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

static struct passgen_env passgen_env_default = {
    .find_complexity = false,
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

    size_t buffer[4];
    buffer[0] = '\\';
    size_t bytes = 0;
    switch(codepoint) {
        case '"':
            buffer[1] = '"';
            bytes = 2;
            break;
        case '\\':
            buffer[1] = '\\';
            bytes = 2;
            break;
        case '\b':
            buffer[1] = 'b';
            bytes = 2;
            break;
        case '\f':
            buffer[1] = 'f';
            bytes = 2;
            break;
        case '\r':
            buffer[1] = 'r';
            bytes = 2;
            break;
        case '\n':
            buffer[1] = 'n';
            bytes = 2;
            break;
        case '\t':
            buffer[1] = 't';
            bytes = 2;
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
    struct passgen_pattern *pattern,
    passgen_random *rand,
    struct passgen_env *env,
    uint32_t *buffer,
    size_t len) {
    struct fillpos fillpos = {
        .buffer = buffer,
        .len = len,
        .cur = 0,
    };

    int ret = passgen_generate(
        pattern,
        rand,
        env,
        &fillpos,
        passgen_generate_write_buffer);

    if(0 != ret) {
        return 0;
    }

    return fillpos.cur;
}

size_t passgen_generate_fill_utf8(
    struct passgen_pattern *pattern,
    passgen_random *rand,
    struct passgen_env *env,
    uint8_t *buffer,
    size_t len) {
    struct fillpos_utf8 fillpos = {
        .buffer = buffer,
        .len = len,
        .cur = 0,
    };

    int ret = passgen_generate(
        pattern,
        rand,
        env,
        &fillpos,
        passgen_generate_write_buffer_utf8);

    if(0 != ret) {
        return 0;
    }

    return fillpos.cur;
}

size_t passgen_generate_fill_json_utf8(
    struct passgen_pattern *pattern,
    passgen_random *rand,
    struct passgen_env *env,
    uint8_t *buffer,
    size_t len) {
    struct fillpos_utf8 fillpos = {
        .buffer = buffer,
        .len = len,
        .cur = 0,
    };

    int ret = passgen_generate(
        pattern,
        rand,
        env,
        &fillpos,
        passgen_generate_write_buffer_json_utf8);

    if(0 != ret) {
        return 0;
    }

    return fillpos.cur;
}

size_t passgen_generate_repeat(
    passgen_random *rand,
    struct passgen_env *env,
    struct passgen_pattern_repeat *repeat) {
    size_t difference = repeat->max - repeat->min;

    // if there is no difference to pick, just return here
    if(0 == difference) {
        return repeat->min;
    }

    // get random number to choose from the range
    size_t choice = passgen_random_u64_max(rand, difference + 1);

    // keep track of complexity
    if(env->find_complexity) {
        env->complexity *= difference + 1;
    }

    return repeat->min + choice;
}

int passgen_generate_set(
    struct passgen_pattern_set *set,
    passgen_random *rand,
    struct passgen_env *env,
    void *data,
    passgen_generate_cb *func) {
    // if this set is empty, we're done.
    if(set->items.len == 0) {
        return 0;
    }

    // compute number of possible codepoints
    // TODO: generate this on the fly or on demand?
    size_t possible = set->choices_list[set->items.len - 1];

    passgen_assert(possible != 0);

    size_t choice = passgen_random_u64_max(rand, possible);

    // keep track of complexity
    if(env->find_complexity) {
        env->complexity *= possible;
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

    struct passgen_pattern_range *range = passgen_stack_get(&set->items, num);

    return func(data, range->start + choice);
}

int passgen_generate_character(
    passgen_chars_t *character,
    passgen_random *rand,
    struct passgen_env *env,
    void *data,
    passgen_generate_cb *func) {
    (void) rand;
    (void) env;

    passgen_assert(character->count > 0);
    passgen_assert(character->count < 8);

    for(size_t i = 0; i < character->count; i++) {
        int ret = func(data, character->codepoints[i]);

        if(ret != 0) {
            return ret;
        }
    }

    return 0;
}

int passgen_generate_special_pronounceable(
    struct passgen_pattern_special *special,
    passgen_random *rand,
    struct passgen_env *env,
    void *data,
    passgen_generate_cb *func) {
    passgen_hashmap_entry *entry =
        passgen_hashmap_lookup(&env->wordlists, special->parameters);
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
    double *complexity = env->find_complexity ? &env->complexity : NULL;
    do {
        word[pos] = passgen_markov_generate(
            markov,
            &word[pos - markov->level],
            rand,
            complexity);
        pos++;
    } while(word[pos - 1]);

    pos = markov->level;
    while(word[pos]) {
        func(data, word[pos]);
        pos++;
    }

    return 0;
}

int passgen_generate_special_wordlist(
    struct passgen_pattern_special *special,
    passgen_random *rand,
    struct passgen_env *env,
    void *data,
    passgen_generate_cb *func) {
    passgen_hashmap_entry *entry =
        passgen_hashmap_lookup(&env->wordlists, special->parameters);
    if(!entry) {
        return -1;
    }
    passgen_wordlist *wordlist = entry->value;
    if(!wordlist->parsed) {
        passgen_wordlist_parse(wordlist);
    }
    const char *word = passgen_wordlist_random(wordlist, rand);
    while(*word) {
        func(data, *word);
        word++;
    }

    if(env->find_complexity) {
        env->complexity *= passgen_wordlist_count(wordlist);
    }

    return 0;
}

int passgen_generate_special(
    struct passgen_pattern_special *special,
    passgen_random *rand,
    struct passgen_env *env,
    void *data,
    passgen_generate_cb *func) {
    switch(special->kind) {
        case PASSGEN_PATTERN_SPECIAL_MARKOV:
            return passgen_generate_special_pronounceable(
                special,
                rand,
                env,
                data,
                func);
            break;
        case PASSGEN_PATTERN_SPECIAL_WORDLIST:
            return passgen_generate_special_wordlist(
                special,
                rand,
                env,
                data,
                func);
            break;
        default:
            passgen_assert(false);
            break;
    }
    return 0;
}

int passgen_generate_item(
    struct passgen_pattern_item *item,
    passgen_random *rand,
    struct passgen_env *env,
    void *data,
    passgen_generate_cb *func) {
    // if it is a maybe (has a question mark following it), decide first if we
    // want to emit it or not.
    if(item->maybe) {
        if(!passgen_random_bool(rand)) {
            return 0;
        }
    }

    // compute random number of repetitions
    size_t reps = passgen_generate_repeat(rand, env, &item->repeat);

    for(size_t i = 0; i < reps; i++) {
        int ret;
        switch(item->kind) {
            case PASSGEN_PATTERN_SET:
                ret = passgen_generate_set(
                    &item->data.set,
                    rand,
                    env,
                    data,
                    func);
                break;
            case PASSGEN_PATTERN_CHAR:
                ret = passgen_generate_character(
                    &item->data.chars,
                    rand,
                    env,
                    data,
                    func);
                break;
            case PASSGEN_PATTERN_SPECIAL:
                ret = passgen_generate_special(
                    &item->data.special,
                    rand,
                    env,
                    data,
                    func);
                break;
            case PASSGEN_PATTERN_GROUP:
                ret = passgen_generate_group(
                    &item->data.group,
                    rand,
                    env,
                    data,
                    func);
                break;
            default:
                passgen_assert(false);
                break;
        }

        // exit early on error
        if(ret != 0) return ret;
    }

    // unreachable
    return 0;
}

int passgen_generate_segment(
    struct passgen_pattern_segment *segment,
    passgen_random *rand,
    struct passgen_env *env,
    void *data,
    passgen_generate_cb *func) {
    for(size_t i = 0; i < segment->items.len; i++) {
        struct passgen_pattern_item *item =
            passgen_stack_get(&segment->items, i);

        int ret = passgen_generate_item(item, rand, env, data, func);

        if(0 != ret) {
            return ret;
        }
    }

    return 0;
}

int passgen_generate_group(
    struct passgen_pattern_group *group,
    passgen_random *rand,
    struct passgen_env *env,
    void *data,
    passgen_generate_cb *func) {
    // choose random segment from segments
    size_t segment = passgen_random_u64_max(rand, group->segments.len);

    // keep track of complexity
    if(env->find_complexity) {
        env->complexity *= group->segments.len;
    }

    // get segment from array
    struct passgen_pattern_segment *segments;
    segments = passgen_stack_get(&group->segments, segment);

    return passgen_generate_segment(segments, rand, env, data, func);
}

int passgen_generate(
    struct passgen_pattern *pattern,
    passgen_random *rand,
    struct passgen_env *env,
    void *data,
    passgen_generate_cb *func) {
    /* use default env if none was supplied. this should be relatively sane. */
    if(!env) {
        env = &passgen_env_default;
    }

    if(env->find_complexity) {
        env->complexity = 1;
    }

    return passgen_generate_group(&pattern->group, rand, env, data, func);
}
