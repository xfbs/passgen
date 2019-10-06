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

int32_t passgen_pronounceable_choose(struct markov2 *list, int32_t a, int32_t b);
