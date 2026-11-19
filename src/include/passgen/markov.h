/// @file markov.h
/// @author Patrick M. Elsen <pelsen@xfbs.net>
/// @brief Markov chain data structure and functions.
///
/// The data structures and functions in this file are used to generate
/// pronounceable phrases by loading in a word list, generating a Markov chain
/// from it, and then using the Markov chain to generate sequences that are
/// pronounceable (in the language of the word list) but mostly not real words.
///
/// The chain is a tree: the nodes are keyed by the previous codepoints, one
/// level for each, and the leaves have the counts of the codepoints which
/// followed them. Both keep their entries in arrays which are sorted by
/// codepoint, which are searched with a binary search, and which grow by one
/// entry at a time. This needs as little memory as possible, which matters
/// for longer chains, which have many nodes with few entries each.
#pragma once
#include "passgen/random.h"
#include <passgen/error.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/// A codepoint which followed a prefix, and how often.
typedef struct {
    /// The codepoint, or zero for the end of the word.
    uint32_t codepoint;
    /// How often it followed the prefix, with the weights of the words.
    uint32_t count;
} passgen_markov_frequency;

/// Leaf in the Markov chain: which codepoints followed a prefix, and how
/// often, sorted by codepoint.
typedef struct {
    /// The sum of the counts.
    size_t total_count;
    /// How many codepoints there are.
    size_t len;
    /// The codepoints and their counts, sorted by codepoint.
    passgen_markov_frequency entries[];
} passgen_markov_leaf;

struct passgen_markov_node;

/// What a node of the Markov chain points to: another node, or in the last
/// level, a leaf.
typedef union {
    struct passgen_markov_node *node;
    passgen_markov_leaf *leaf;
} passgen_markov_child;

/// Intermediate node in the Markov chain: a child for every codepoint which
/// can be at this position of a prefix, sorted by codepoint.
///
/// The children come first, and are followed by the codepoints (see @ref
/// passgen_markov_node_codepoints), in the same allocation. Keeping them
/// apart rather than in pairs saves the padding that pairs would need.
typedef struct passgen_markov_node {
    /// How many children there are.
    size_t len;
    /// The children, in the order of their codepoints.
    passgen_markov_child children[];
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
/// The @ref level of a Markov chain is how many of the previous codepoints
/// it takes into account when generating the next one. The command-line tool
/// calls this the length of the Markov chain.
typedef struct {
    /// Length of codepoint tuples that this Markov chains stores.
    size_t level;
    /// How many words are in this Markov chain
    size_t count;
    /// Root node of markov chain, or NULL if nothing was added yet.
    passgen_markov_node *root;
} passgen_markov;

/// The codepoints of a node, which follow its children.
///
/// @memberof passgen_markov_node
static inline uint32_t *
passgen_markov_node_codepoints(const passgen_markov_node *node) {
    return (uint32_t *) &node->children[node->len];
}

/// Allocates a new Markov chain node, without any children.
///
/// @returns The node, or NULL if there is no memory for it
/// @memberof passgen_markov_node
passgen_markov_node *passgen_markov_node_new(void);

/// Free a Markov chain node, and its children.
///
/// The parameter @ref level is how many levels of nodes there are below and
/// including this one, so that it knows whether its children are nodes or
/// leaves (for a level of one).
///
/// @memberof passgen_markov_node
void passgen_markov_node_free(passgen_markov_node *node, size_t level);

/// Find the child of a node for a codepoint.
///
/// @returns The child, or NULL if there is none for this codepoint
/// @memberof passgen_markov_node
passgen_markov_child *
passgen_markov_node_child(const passgen_markov_node *node, uint32_t codepoint);

/// Insert a new word into this Markov chain node.
///
/// This will recursively insert the word into the appropriate child nodes. It
/// needs to know the @ref length of the word (this is equal to the @ref level
/// of the Markov chain, minus the depth it has already traversed, plus one)
/// and the weight of the word.
///
/// The node is replaced if it has to grow, and allocated if it is NULL.
///
/// @returns @ref PASSGEN_OK, or @ref PASSGEN_ERROR_OUT_OF_MEMORY. In that
///   case, some of the word may have been inserted, and the node is still
///   valid.
/// @memberof passgen_markov_node
passgen_status passgen_markov_node_insert_word(
    passgen_markov_node **node,
    const uint32_t *sequence,
    size_t length,
    size_t weight);

/// Allocates a new Markov leaf, without any codepoints.
///
/// @returns The leaf, or NULL if there is no memory for it
/// @memberof passgen_markov_leaf
passgen_markov_leaf *passgen_markov_leaf_new(void);

/// Free a Markov leaf node.
///
/// @memberof passgen_markov_leaf
void passgen_markov_leaf_free(passgen_markov_leaf *leaf);

/// Add to the count of a codepoint in a Markov leaf.
///
/// This returns a pointer to the leaf because it may need to be reallocated
/// to make room for the codepoint. If there is no memory for that, it returns
/// NULL, and the leaf is unchanged.
///
/// @memberof passgen_markov_leaf
passgen_markov_leaf *passgen_markov_leaf_insert(
    passgen_markov_leaf *leaf,
    uint32_t codepoint,
    size_t weight);

/// Given a codepoint, return a count or zero if the codepoint does not exist.
///
/// @memberof passgen_markov_leaf
uint32_t
passgen_markov_leaf_count(const passgen_markov_leaf *leaf, uint32_t codepoint);

/// Initialize new markov chain with a given level
///
/// This does not allocate anything until a word is added.
///
/// @memberof passgen_markov
void passgen_markov_init(passgen_markov *markov, uint8_t level);

/// Add a word to a markov chain
///
/// @returns @ref PASSGEN_OK, or @ref PASSGEN_ERROR_OUT_OF_MEMORY. In that
///   case, some of the word may have been added, and the chain should be
///   freed.
/// @memberof passgen_markov
passgen_status passgen_markov_add(
    passgen_markov *markov,
    const uint32_t *word,
    size_t word_len,
    size_t weight);

/// Generate the next character at random using the Markov chain and a @ref
/// passgen_random instance.
///
/// The codepoints which can follow are chosen from in the order of their
/// codepoints, so what is generated from a deterministic source of
/// randomness only depends on the words. If only one codepoint can follow,
/// this takes no randomness.
///
/// @param current Must contain markov->level amount of previous chars
/// @param markov Must be a parsed markov chain
/// @param random Must be a valid randomness instance
/// @param entropy If not NULL, the entropy of this choice is added to it, in
///   bits: the logarithm to base two of the number of choices there were,
///   divided by how many of them result in the codepoint which is returned.
/// @return The generated next character
/// @memberof passgen_markov
uint32_t passgen_markov_generate(
    passgen_markov *markov,
    const uint32_t *current,
    passgen_random *random,
    double *entropy);

/// Free a markov chain
///
/// @memberof passgen_markov
void passgen_markov_free(passgen_markov *markov);
