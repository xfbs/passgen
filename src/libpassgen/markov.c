#include "passgen/markov.h"
#include "passgen/assert.h"
#include <stdlib.h>
#include <string.h>
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define SATURATING_SUB(a, b) ((a) - MIN(a, b))

const struct passgen_markov0 *passgen_markov2_choose(
    const struct passgen_markov3 *list,
    passgen_random_t *rand,
    int32_t p1,
    int32_t p2) {
    const struct passgen_markov2 *list2 = passgen_markov3_find(list, p1);
    if(!list2) {
        return NULL;
    }

    const struct passgen_markov1 *list1 = passgen_markov2_find(list2, p2);
    if(!list1) {
        return NULL;
    }

    size_t choices = list1->frequency_sum;
    size_t nchoice = passgen_random_u64_max(rand, choices);

    return passgen_markov1_find(list1, nchoice);
}

size_t passgen_markov2_generate(
    const struct passgen_markov3 *list,
    passgen_random_t *rand,
    int32_t *buf,
    size_t len) {
    int32_t p1 = 0, p2 = 0;
    size_t pos = 0;

    do {
        const struct passgen_markov0 *choice =
            passgen_markov2_choose(list, rand, p1, p2);
        passgen_assert(choice);

        /* save codepoint */
        if(choice->codepoint) {
            buf[pos] = choice->codepoint;
        }
        pos += 1;

        p1 = p2;
        p2 = choice->codepoint;
    } while(p2 && pos < len);

    return pos - 1;
}

static int markov1_find(const void *key_p, const void *item_p) {
    const int32_t *key = key_p;
    const struct passgen_markov1 *item = item_p;

    if(*key < item->codepoint) {
        return -1;
    } else if(*key > item->codepoint) {
        return 1;
    } else {
        return 0;
    }
}

const struct passgen_markov1 *
passgen_markov2_find(const struct passgen_markov2 *list, int32_t codepoint) {
    const struct passgen_markov1 *result;

    result = bsearch(
        &codepoint,
        list->list,
        list->list_len,
        sizeof(struct passgen_markov1),
        markov1_find);

    return result;
}

static int markov_find(const void *key_p, const void *item_p) {
    const int32_t *key = key_p;
    const struct passgen_markov2 *item = item_p;

    if(*key < item->codepoint) {
        return -1;
    } else if(*key > item->codepoint) {
        return 1;
    } else {
        return 0;
    }
}

const struct passgen_markov2 *
passgen_markov3_find(const struct passgen_markov3 *list, int32_t codepoint) {
    const struct passgen_markov2 *result;

    result = bsearch(
        &codepoint,
        list->list,
        list->list_len,
        sizeof(struct passgen_markov2),
        markov_find);

    return result;
}

const struct passgen_markov0 *
passgen_markov1_find(const struct passgen_markov1 *list, size_t choice) {
    for(size_t i = 0; i < list->list_len; i++) {
        if(choice < list->list[i].frequency) {
            return &list->list[i];
        }

        choice -= list->list[i].frequency;
    }

    return NULL;
}

#define MARKOV_LIST_INITIAL    32
#define MARKOV_LIST_MULTIPLIER 2

// Initialize new markov chain with a given level
void passgen_markov_init(passgen_markov *markov, uint8_t level) {
    passgen_assert(level);
    memset(markov, 0, sizeof(*markov));
    markov->level = level;
}

void passgen_markov_insert(
    passgen_markov *markov,
    const uint32_t *sequence,
    size_t weight) {
}

// Add a word to a markov chain
void passgen_markov_add(
    passgen_markov *markov,
    const uint32_t *word,
    size_t word_len,
    size_t weight) {
    // insert start sequences
    size_t sequence_len = markov->level + markov->level;
    uint32_t sequence[sequence_len];
    memset(sequence, 0, sizeof(sequence[0]) * sequence_len);
    memcpy(&sequence[markov->level], word, sizeof(uint32_t) * MIN(markov->level, word_len));
    for(size_t i = 0; i < markov->level; i++) {
        passgen_markov_insert(markov, &sequence[i], weight);
    }
    for(size_t i = 0; i < SATURATING_SUB(word_len, markov->level); i++) {
        passgen_markov_insert(markov, &word[i], weight);
    }
    memset(sequence, 0, sizeof(sequence[0]) * sequence_len);
    memcpy(sequence, &word[SATURATING_SUB(word_len, markov->level)], sizeof(uint32_t) * MIN(markov->level, word_len));
    for(size_t i = 0; i < MIN(markov->level, word_len); i++) {
        passgen_markov_insert(markov, &sequence[i], weight);
    }
}

// Free a markov chain
void passgen_markov_free(passgen_markov *markov) {
}
