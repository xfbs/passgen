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
struct passgen_markov_node;

struct passgen_markov {
  size_t level;
  struct passgen_markov_node *root;
};

struct passgen_markov_node {
  size_t capacity;
  union {
    size_t cumulative;
    union {
      uint32_t codepoint;
      struct passgen_markov *node;
    } data[0];
  } data[0];
};

typedef struct passgen_markov passgen_markov;
typedef struct passgen_markov_node passgen_markov_node;
