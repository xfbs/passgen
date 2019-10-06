#pragma once
#include <stddef.h>
#include <stdint.h>

struct markov0 {
    int32_t codepoint;
    size_t frequency;
};

struct markov1 {
    int32_t codepoint;
    size_t frequency_sum;
    struct markov0 *list;
    size_t list_len;
};

struct markov2 {
    int32_t codepoint;
    struct markov1 *list;
    size_t list_len;
};

struct markov {
    struct markov2 *list;
    size_t list_len;
};

/* given a markov chain and two codepoints, choose the next codepoint. */
int32_t passgen_pronounceable_choose2(struct markov2 *list, int32_t a, int32_t b);

/* use a binary search to find a codepoint in a markov2 chain. */
struct markov1 *
passgen_pronounceable_find1(
        struct markov2 *list,
        int32_t codepoint);

/* use binary search to find a codepoint in a markov chain. returns NULL on
 * error. */
struct markov2 *
passgen_pronounceable_find2(
        struct markov *list,
        int32_t codepoint);

/* list of symbols of all markov chains */
extern struct markov passgen_pronounceable_english;
