#pragma once
#include "passgen/data/markov.h"
#include "passgen/random.h"
#include <stddef.h>
#include <stdint.h>

// generate a word from a markov2 chain.
size_t passgen_markov2_generate(
    const struct passgen_markov3 *list,
    passgen_random_t *rand,
    int32_t *buf,
    size_t len);

// given a markov chain and two codepoints, choose the next codepoint. */
const struct passgen_markov0 *passgen_markov2_choose(
    const struct passgen_markov3 *list,
    passgen_random_t *rand,
    int32_t a,
    int32_t b);

// given a final markov chain list, find the right codepoint.
const struct passgen_markov0 *
passgen_markov1_find(const struct passgen_markov1 *list, size_t choice);

// use a binary search to find a codepoint in a markov2 chain.
const struct passgen_markov1 *
passgen_markov2_find(const struct passgen_markov2 *list, int32_t codepoint);

// use binary search to find a codepoint in a markov chain. returns NULL on
// error.
const struct passgen_markov2 *
passgen_markov3_find(const struct passgen_markov3 *list, int32_t codepoint);
