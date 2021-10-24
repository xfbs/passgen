#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/// This is essentially a sorted list of codepoint-count tuples.
/// During insertion, these are sorted by the codepoint, however when finalizing
/// the sort order might be changed to being sorted by count to allow for faster
/// generation.
struct passgen_markov_leaf {
  size_t cumulative;
  size_t capacity;
  union {
    uint32_t count[1];
    uint32_t codepoint[1];
  } data[0];
};

struct passgen_markov;

/// Intermediate nodes in the markov chain tree. Implemented as custom hash map
/// where the keys are codepoints and the values are nodes or leafs.
struct passgen_markov_node {
  size_t capacity;
  union {
    size_t cumulative[1];
    uint32_t codepoint[1];
    struct passgen_markov *child[1];
  } data[0];
};

struct passgen_markov {
  size_t level;
  size_t count;
  struct passgen_markov_node *root;
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
