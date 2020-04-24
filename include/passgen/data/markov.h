#pragma once
#include <stdint.h>
#include <stddef.h>

struct passgen_markov0 {
  int32_t codepoint;
  size_t frequency;
};

struct passgen_markov1 {
  int32_t codepoint;
  size_t frequency_sum;
  const struct markov0 *list;
  size_t list_len;
};

struct passgen_markov2 {
  int32_t codepoint;
  const struct markov1 *list;
  size_t list_len;
};

struct passgen_markov {
  const struct markov2 *list;
  size_t list_len;
};
