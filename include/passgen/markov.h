/// @file markov.h
/// @author Patrick M. Elsen <pelsen@xfbs.net>
/// @brief Markov chain data structure and functions.
///
/// The data structures and functions in this file are used to generate
/// pronounceable phrases by loading in a word list, generating a Markov chain
/// from it, and then using the Markov chain to generate sequences that are
/// pronounceable in English but not real words.
#pragma once
#include "passgen/util/random.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define passgen_markov_node_child(node, codepoint) \
    node->entries[(node->capacity + 1) / 2].child[codepoint % node->capacity]

#define passgen_markov_node_codepoint(node, index) \
    node->entries[0].codepoint[index % node->capacity]

/// Leaf node in the Markov chain. This contains a sorted list of
/// codepoint-count tuples.  During insertion, these are sorted by the
/// codepoint. When finalizing the Markov chain, they might be reordered to be
/// sorted by the frequency to allow for more efficient generation.
typedef struct {
    size_t total_count;
    size_t capacity;
    union {
        uint32_t count[1];
        uint32_t codepoint[1];
    } entries[0];
} passgen_markov_leaf;

/// Intermediate nodes in the markov chain tree. Implemented as custom hash map
/// where the keys are codepoints and the values are nodes or leafs.
typedef struct passgen_markov_node {
  size_t capacity;
  union {
    uint32_t codepoint[1];
    union {
        struct passgen_markov_node *node;
        passgen_markov_leaf *leaf;
    } child[1];
  } entries[0];
} passgen_markov_node;

/// Markov chain, used to generate pronounceable random words
///
/// Contains a tree of unicode codepoints and frequency data.
///
/// This data structure answers the question "given the previous characters x
/// and y, what are the probabilities for the next character?". Using
/// #passgen_markov_add, you can add words to it. Given a sequence of
/// characters, using #passgen_markov_generate you can generate a new
/// character at random.
///
/// The @ref length of a Markov chain is the length of tuples that it looks at.
typedef struct {
    /// Length of codepoint tuples that this Markov chains stores.
    size_t level;
    /// How many words are in this Markov chain
    size_t count;
    /// Root node of markov chain
    passgen_markov_node *root;
} passgen_markov;


/// Allocates a new Markov chain node with a size determined by the @ref
/// size_index.
///
/// Markov chain nodes only exist in specific, discrete sizes that are prime
/// numbers.  This is because they implement a kind of hash map and that makes
/// them more efficient.  The @ref size_index is an index into a static array
/// of precomputed prime sizes.
///
/// @memberof passgen_markov_node
passgen_markov_node *passgen_markov_node_new(size_t size_index);

/// Free a Markov chain node.
///
/// The parameter @ref level is the level of the Markov chain, this needs to be
/// passed so that the method knows how deep to free data.
///
/// @memberof passgen_markov_node
void passgen_markov_node_free(passgen_markov_node *node, size_t level);

/// Insert a new codepoint into a Markov chain node.
///
/// This returns a pointer to a passgen_markov_node because the node may need
/// to be reallocated if it would otherwise overflow.
///
/// @memberof passgen_markov_node
passgen_markov_node *passgen_markov_node_insert(passgen_markov_node *node, uint32_t codepoint);

/// Insert a new word into this Markov chain node.
///
/// This will recursively insert the word into the appropriate child nodes. It
/// needs to know the @ref length of the word (this is equal to the @ref level
/// of the Markov chain, minus the depth it has already traversed) and the
/// weight of the word.
///
/// @memberof passgen_markov_node
passgen_markov_node *passgen_markov_node_insert_word(
    passgen_markov_node *node,
    const uint32_t *sequence,
    size_t length,
    size_t weight);

/// Allocates a new Markov leaf node with a size determined by the @ref
/// size_index.
///
/// Markov leaf nodes only exist in specific, discrete sizes that are prime
/// numbers.  This is because they implement a kind of hash map that makes them
/// more efficient.  The @ref size_index is an index into a static array of
/// precomputed prime sizes.
///
/// @memberof passgen_markov_leaf
passgen_markov_leaf *passgen_markov_leaf_new(size_t size_index);

/// Free a Markov leaf node.
///
/// @memberof passgen_markov_leaf
void passgen_markov_leaf_free(passgen_markov_leaf *leaf);

/// Insert a final codepoint and weight into a Markov leaf node.
///
/// @memberof passgen_markov_leaf
passgen_markov_leaf *passgen_markov_leaf_insert(passgen_markov_leaf *leaf, uint32_t codepoint, size_t weight);

/// Given a codepoint, return a count or zero if the codepoint does not exist.
///
/// @memberof passgen_markov_leaf
uint32_t passgen_markov_leaf_count(passgen_markov_leaf *leaf, uint32_t codepoint);

/// Initialize new markov chain with a given level
///
/// @memberof passgen_markov
void passgen_markov_init(passgen_markov *markov, uint8_t level);

/// Add a word to a markov chain
///
/// @memberof passgen_markov
void passgen_markov_add(
    passgen_markov *markov,
    const uint32_t *word,
    size_t word_len,
    size_t weight);

/// Generate the next character at random using the Markov chain and a @ref passgen_random instance.
/// @param current Must contain markov->level amount of previous chars
/// @param markov Must be a parsed markov chain
/// @param random Must be a valid randomness instance
/// @param complexity Will be multiplied with the complexity of the choice if it not NULL.
/// @return The generated next character
/// @memberof passgen_markov
uint32_t passgen_markov_generate(
    passgen_markov *markov,
    const uint32_t *current,
    passgen_random *random,
    double *complexity);

/// Free a markov chain
///
/// @memberof passgen_markov
void passgen_markov_free(passgen_markov *markov);
