#pragma once
#include "passgen/data/markov.h"
#include "passgen/random.h"
#include <stddef.h>
#include <stdint.h>

// Initialize new markov chain with a given level
void passgen_markov_init(struct passgen_markov *markov, uint8_t level);

// Add a word to a markov chain
void passgen_markov_add(
    passgen_markov *markov,
    const uint32_t *word,
    size_t word_len,
    size_t weight);

/// Given markov->level amount of previous chars, generate the next character at random.
uint32_t passgen_markov_generate(
    passgen_markov *markov,
    const uint32_t *current,
    passgen_random *random);

// Free a markov chain
void passgen_markov_free(passgen_markov *markov);
