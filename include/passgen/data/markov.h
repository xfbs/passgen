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
  const struct passgen_markov0 *list;
  size_t list_len;
};

struct passgen_markov2 {
  int32_t codepoint;
  const struct passgen_markov1 *list;
  size_t list_len;
};

struct passgen_markov3 {
  int32_t codepoint;
  const struct passgen_markov2 *list;
  size_t list_len;
};

struct passgen_markov_leaf {
  uint32_t codepoint;
  size_t frequency;
};

struct passgen_markov;
struct passgen_markov {
  uint8_t level;

  size_t cumulative;
  uint32_t codepoint;
  union {
    struct passgen_markov *node;
    struct passgen_markov_leaf *leaf;
  } children;
  uint32_t children_len;
};
