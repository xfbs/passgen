#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define passgen_markov_node_child(node, codepoint) \
    node->entries[(node->capacity + 1) / 2].child[codepoint % node->capacity]

#define passgen_markov_node_codepoint(node, index) \
    node->entries[0].codepoint[index % node->capacity]

#define passgen_markov_leaf_count(leaf, codepoint) \
    leaf->entries[leaf->capacity].count[codepoint % leaf->capacity]

#define passgen_markov_leaf_codepoint(leaf, index) \
    leaf->entries[0].codepoint[index % leaf->capacity]

/// This is essentially a sorted list of codepoint-count tuples.
/// During insertion, these are sorted by the codepoint, however when finalizing
/// the sort order might be changed to being sorted by count to allow for faster
/// generation.
struct passgen_markov_leaf {
  size_t total_count;
  size_t capacity;
  union {
    uint32_t count[1];
    uint32_t codepoint[1];
  } entries[0];
};

struct passgen_markov;

/// Intermediate nodes in the markov chain tree. Implemented as custom hash map
/// where the keys are codepoints and the values are nodes or leafs.
struct passgen_markov_node {
  size_t capacity;
  union {
    uint32_t codepoint[1];
    union {
        struct passgen_markov_node *node;
        struct passgen_markov_leaf *leaf;
    } child[1];
  } entries[0];
};

struct passgen_markov {
  size_t level;
  size_t count;
  struct passgen_markov_node *root;
};

typedef struct passgen_markov passgen_markov;
typedef struct passgen_markov_node passgen_markov_node;
typedef struct passgen_markov_leaf passgen_markov_leaf;

passgen_markov_node *passgen_markov_node_new(size_t size_index);
void passgen_markov_node_free(passgen_markov_node *node, size_t level);
passgen_markov_node *passgen_markov_node_insert(passgen_markov_node *node, uint32_t codepoint);
passgen_markov_node *passgen_markov_node_insert_word(passgen_markov_node *node, const uint32_t *sequence, size_t length, size_t weight);

passgen_markov_leaf *passgen_markov_leaf_new(size_t size_index);
void passgen_markov_leaf_free(passgen_markov_leaf *leaf);
passgen_markov_leaf *passgen_markov_leaf_insert(passgen_markov_leaf *leaf, uint32_t codepoint, size_t weight);
