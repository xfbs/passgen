#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

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
  size_t cumulative;
  struct passgen_markov_node *root;
};

struct passgen_markov_node {
  size_t capacity;
  union {
    size_t cumulative[1];
    uint32_t codepoint[1];
    struct passgen_markov *child[1];
  } data[0];
};

typedef struct passgen_markov passgen_markov;
typedef struct passgen_markov_node passgen_markov_node;

size_t passgen_markov_node_size(size_t capacity, bool leaf);
passgen_markov_node *passgen_markov_node_new(size_t capacity, bool leaf);
passgen_markov_node *passgen_markov_node_resize(passgen_markov_node *node, size_t capacity, bool leaf);
uint32_t *passgen_markov_node_codepoint(passgen_markov_node *node, size_t index);
size_t *passgen_markov_node_cumulative(passgen_markov_node *node, size_t index);
passgen_markov_node **passgen_markov_node_child(passgen_markov_node *node, size_t index);
void passgen_markov_node_free(passgen_markov_node *node, size_t level);
