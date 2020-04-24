#pragma once
#include "passgen/data/markov.h"
#include "passgen/random.h"
#include <stddef.h>
#include <stdint.h>

size_t passgen_pronounceable2(
    const struct passgen_markov *list,
    passgen_random_t *rand,
    int32_t *buf,
    size_t len);

/* given a markov chain and two codepoints, choose the next codepoint. */
const struct passgen_markov0 *passgen_pronounceable_choose(
    const struct passgen_markov *list,
    passgen_random_t *rand,
    int32_t a,
    int32_t b);

/* use a binary search to find a codepoint in a markov2 chain. */
const struct passgen_markov1 *
passgen_pronounceable_find1(const struct passgen_markov2 *list, int32_t codepoint);

/* use binary search to find a codepoint in a markov chain. returns NULL on
 * error. */
const struct passgen_markov2 *
passgen_pronounceable_find2(const struct passgen_markov *list, int32_t codepoint);

/* given a final markov chain list, find the right codepoint. */
const struct passgen_markov0 *
passgen_pronounceable_find(const struct passgen_markov1 *list, size_t choice);

/* list of symbols of all markov chains */
extern const struct passgen_markov passgen_pronounceable_english;
extern const struct passgen_markov passgen_pronounceable_latin;
extern const struct passgen_markov passgen_pronounceable_german;
