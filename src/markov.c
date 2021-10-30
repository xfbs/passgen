#include "passgen/markov.h"
#include "passgen/assert.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define SATURATING_SUB(a, b) ((a) - MIN(a, b))

#define MARKOV_LEAF_INITIAL    4
#define MARKOV_LEAF_MULTIPLIER 2

static const size_t markov_sizes[] = {3, 7, 17, 37, 79, 163, 331, 673, 1361, 2729, 5471, 10949, 21911, 43853, 87719, 175447, 350899, 701819, 0};

size_t passgen_markov_node_size(size_t capacity) {
    // we use capacity + 1 there to make sure it's a round number, such that the
    // child pointers are not misaligned.
    return sizeof(passgen_markov_node) + (capacity + 1) * sizeof(uint32_t) + capacity * sizeof(passgen_markov_node *);
}

size_t passgen_markov_leaf_size(size_t capacity) {
    return sizeof(passgen_markov_leaf) + capacity * (sizeof(uint32_t) + sizeof(uint32_t));
}

passgen_markov_node *passgen_markov_node_new(size_t size_index) {
    size_t capacity = markov_sizes[size_index];
    size_t size = passgen_markov_node_size(capacity);
    passgen_markov_node *node = calloc(1, size);
    memset(node, 0, size);
    node->capacity = capacity;
    return node;
}

passgen_markov_leaf *passgen_markov_leaf_new(size_t capacity) {
    size_t size = passgen_markov_leaf_size(capacity);
    passgen_markov_leaf *leaf = calloc(1, size);
    memset(leaf, 0, size);
    leaf->capacity = capacity;
    return leaf;
}

void passgen_markov_leaf_free(passgen_markov_leaf *leaf) {
    free(leaf);
}

void passgen_markov_node_free(passgen_markov_node *node, size_t level) {
    for(size_t i = 0; i < node->capacity; i++) {
        void *child = passgen_markov_node_child(node, i).leaf;
        if(child) {
            if(!level) {
                passgen_markov_leaf_free(child);
            } else {
                passgen_markov_node_free(child, level - 1);
            }
        }
    }

    free(node);
}

// Initialize new markov chain with a given level
void passgen_markov_init(passgen_markov *markov, uint8_t level) {
    passgen_assert(level);
    memset(markov, 0, sizeof(*markov));
    markov->level = level;
    markov->root = passgen_markov_node_new(0);
}

passgen_markov_leaf *passgen_markov_leaf_insert(passgen_markov_leaf *child, uint32_t codepoint, size_t weight) {
    return child;
}

passgen_markov_node *passgen_markov_node_realloc(passgen_markov_node *node) {
    size_t size_index = 0;
    while(markov_sizes[size_index] < node->capacity) {
        size_index++;
    }

    passgen_markov_node *new = passgen_markov_node_new(size_index + 1);

    // re-insert old elements
    for(size_t i = 0; i < node->capacity; i++) {
        if(passgen_markov_node_child(node, i).node) {
            new = passgen_markov_node_insert(new, passgen_markov_node_codepoint(node, i));
            passgen_markov_node_child(new, i) = passgen_markov_node_child(node, i);
        }
    }

    free(node);

    return new;
}

passgen_markov_node *passgen_markov_node_insert(
    passgen_markov_node *node,
    uint32_t codepoint) {

    while(passgen_markov_node_child(node, codepoint).node && passgen_markov_node_codepoint(node, codepoint) != codepoint) {
        node = passgen_markov_node_realloc(node);
    }

    // set codepoint
    passgen_markov_node_codepoint(node, codepoint) = codepoint;

    return node;
}

passgen_markov_node *passgen_markov_node_insert_word(
    passgen_markov_node *node,
    const uint32_t *sequence,
    size_t length,
    size_t weight) {

    uint32_t codepoint = sequence[0];
    node = passgen_markov_node_insert(node, codepoint);

    if(length == 2) {
        passgen_markov_leaf *child = passgen_markov_node_child(node, codepoint).leaf;

        if(!child) {
            child = passgen_markov_leaf_new(MARKOV_LEAF_INITIAL);
        }

        child = passgen_markov_leaf_insert(child, sequence[1], weight);

        passgen_markov_node_child(node, codepoint).leaf = child;
    } else {
        // retrieve or create child node
        passgen_markov_node *child = passgen_markov_node_child(node, codepoint).node;
        if(!child) {
            child = passgen_markov_node_new(0);
        }

        // insert word into child
        child = passgen_markov_node_insert_word(child, &sequence[1], length - 1, weight);

        // save child
        passgen_markov_node_child(node, codepoint).node = child;
    }

    return node;
}

void passgen_markov_insert(
    passgen_markov *markov,
    const uint32_t *sequence,
    size_t weight) {
    markov->root = passgen_markov_node_insert_word(markov->root, sequence, markov->level + 1, weight);
    markov->count += weight;
}

// Add a word to a markov chain
void passgen_markov_add(
    passgen_markov *markov,
    const uint32_t *word,
    size_t word_len,
    size_t weight) {
    // insert start sequences
    size_t sequence_len = markov->level + markov->level;
    uint32_t sequence[sequence_len];
    memset(sequence, 0, sizeof(sequence[0]) * sequence_len);
    memcpy(&sequence[markov->level], word, sizeof(uint32_t) * MIN(markov->level, word_len));
    for(size_t i = 0; i < markov->level; i++) {
        passgen_markov_insert(markov, &sequence[i], weight);
    }
    for(size_t i = 0; i < SATURATING_SUB(word_len, markov->level); i++) {
        passgen_markov_insert(markov, &word[i], weight);
    }
    memset(sequence, 0, sizeof(sequence[0]) * sequence_len);
    memcpy(sequence, &word[SATURATING_SUB(word_len, markov->level)], sizeof(uint32_t) * MIN(markov->level, word_len));
    for(size_t i = 0; i < MIN(markov->level, word_len); i++) {
        passgen_markov_insert(markov, &sequence[i], weight);
    }
}

// Free a markov chain
void passgen_markov_free(passgen_markov *markov) {
    passgen_markov_node_free(markov->root, markov->level);
}

uint32_t passgen_markov_generate(
    passgen_markov *markov,
    const uint32_t *current,
    passgen_random *random) {
    passgen_markov_node *node = markov->root;
    size_t choices = 0;
    for(size_t i = 0; i < markov->level; i++) {
        size_t pos = 0; // = passgen_markov_node_find(node, current[i]);
        //passgen_assert(*passgen_markov_node_codepoint(node, pos) == current[i]);
        //choices = *passgen_markov_node_cumulative(node, pos);
        //node = *passgen_markov_node_child(node, pos);
    }

    passgen_assert(choices);
    passgen_assert(node);

    size_t choice = passgen_random_u64_max(random, choices);
    for(size_t i = 0; i < node->capacity; i++) {
        size_t cumulative = 0; // *passgen_markov_node_cumulative(node, i);
        if(cumulative > choice) {
            //return *passgen_markov_node_codepoint(node, i);
        } else {
            choice -= cumulative;
        }
    }

    return 0;
}
