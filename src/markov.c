#include "passgen/markov.h"
#include "passgen/assert.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define SATURATING_SUB(a, b) ((a) - MIN(a, b))

#define MARKOV_LEAF_INITIAL    4
#define MARKOV_LEAF_MULTIPLIER 2

/// Sizes for the leaf and node hash maps. These are all prime numbers, with the
/// property that every number is greater than double the previous one. Generated
/// with Ruby with:
///
/// ```ruby
/// require 'prime'
/// p Prime.take(10000).inject([3]){|l, p| l << p if (2*l.last) < p; l}
/// ```
static const size_t markov_sizes[] = {
    3, 7, 17, 37, 79, 163, 331, 673, 1361, 2729, 5471, 10949, 21911, 43853,
    87719, 175447, 350899, 701819, 1403641, 2807303, 5614657, 11229331,
    22458671, 44917381, 0
};

size_t passgen_markov_node_size(size_t capacity) {
    // we use capacity + 1 there to make sure it's a round number, such that the
    // child pointers are not misaligned.
    return sizeof(passgen_markov_node)
        + (capacity + 1) * sizeof(uint32_t)
        + capacity * sizeof(passgen_markov_node *);
}

size_t passgen_markov_leaf_size(size_t capacity) {
    return sizeof(passgen_markov_leaf)
        + capacity * sizeof(uint32_t)
        + capacity * sizeof(uint32_t);
}

passgen_markov_node *passgen_markov_node_new(size_t size_index) {
    size_t capacity = markov_sizes[size_index];
    passgen_assert(capacity);
    size_t size = passgen_markov_node_size(capacity);
    passgen_markov_node *node = calloc(1, size);
    memset(node, 0, size);
    node->capacity = capacity;
    return node;
}

passgen_markov_leaf *passgen_markov_leaf_new(size_t size_index) {
    size_t capacity = markov_sizes[size_index];
    size_t size = passgen_markov_leaf_size(capacity);
    passgen_markov_leaf *leaf = calloc(1, size);
    memset(leaf, 0, size);
    leaf->capacity = capacity;
    leaf->total_count = 0;
    return leaf;
}

void passgen_markov_leaf_free(passgen_markov_leaf *leaf) {
    free(leaf);
}

void passgen_markov_node_free(passgen_markov_node *node, size_t level) {
    for(size_t i = 0; i < node->capacity; i++) {
        void *child = passgen_markov_node_child(node, i).leaf;
        if(child) {
            if(level < 2) {
                passgen_markov_leaf_free(child);
            } else {
                passgen_markov_node_free(child, level - 1);
            }
        }
    }

    free(node);
}

void passgen_markov_node_check(passgen_markov_node *node, size_t level) {
    assert(node->capacity);
    assert(node->capacity % 2);
    for(size_t i = 0; i < node->capacity; i++) {
        void *data = passgen_markov_node_child(node, i).node;
        uint32_t codepoint = passgen_markov_node_codepoint(node, i);
        if(codepoint) {
            assert(data);
        }
        if(data) {
            assert((codepoint % node->capacity) == i);
            if(level > 2) {
                passgen_markov_node_check(data, level - 1);
            }
        }
    }
}

void passgen_markov_check(passgen_markov *markov) {
    passgen_markov_node_check(markov->root, markov->level);
}

// Initialize new markov chain with a given level
void passgen_markov_init(passgen_markov *markov, uint8_t level) {
    passgen_assert(level);
    memset(markov, 0, sizeof(*markov));
    markov->level = level;
    markov->root = passgen_markov_node_new(0);
}

passgen_markov_leaf *passgen_markov_leaf_realloc(passgen_markov_leaf *leaf) {
    size_t size_index = 0;
    while(markov_sizes[size_index] < leaf->capacity) {
        size_index++;
    }

    passgen_markov_leaf *new = passgen_markov_leaf_new(size_index + 1);

    // re-insert old elements
    for(size_t i = 0; i < leaf->capacity; i++) {
        if(passgen_markov_leaf_count(leaf, i)) {
            uint32_t codepoint = passgen_markov_leaf_codepoint(leaf, i);
            uint32_t count = passgen_markov_leaf_count(leaf, i);
            new = passgen_markov_leaf_insert(new, codepoint, count);
        }
    }

    free(leaf);

    return new;
}

passgen_markov_leaf *passgen_markov_leaf_insert(passgen_markov_leaf *leaf, uint32_t codepoint, size_t weight) {
    while(passgen_markov_leaf_count(leaf, codepoint) && passgen_markov_leaf_codepoint(leaf, codepoint) != codepoint) {
        leaf = passgen_markov_leaf_realloc(leaf);
    }

    // set codepoint
    passgen_markov_leaf_codepoint(leaf, codepoint) = codepoint;
    passgen_markov_leaf_count(leaf, codepoint) += weight;
    leaf->total_count += weight;

    return leaf;
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
            uint32_t codepoint = passgen_markov_node_codepoint(node, i);
            new = passgen_markov_node_insert(new, codepoint);
            assert(!passgen_markov_node_child(new, codepoint).node);
            passgen_markov_node_child(new, codepoint).node = passgen_markov_node_child(node, i).node;
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
            child = passgen_markov_leaf_new(0);
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
    size_t sequence_len = 2 * markov->level + 1;
    uint32_t sequence[sequence_len];
    memset(sequence, 0, sizeof(sequence[0]) * sequence_len);
    memcpy(&sequence[markov->level], word, sizeof(uint32_t) * MIN(markov->level, word_len));
    for(size_t i = 0; i < MIN(markov->level, word_len); i++) {
        passgen_markov_insert(markov, &sequence[i], weight);
    }
    for(size_t i = 0; i < SATURATING_SUB(word_len, markov->level); i++) {
        passgen_markov_insert(markov, &word[i], weight);
    }
    memset(sequence, 0, sizeof(sequence[0]) * sequence_len);
    memcpy(
        &sequence[SATURATING_SUB(markov->level, word_len)],
        &word[SATURATING_SUB(word_len, markov->level)],
        sizeof(uint32_t) * MIN(markov->level, word_len)
    );
    passgen_markov_insert(markov, &sequence[0], weight);
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

    for(size_t i = 0; i < markov->level; i++) {
        node = passgen_markov_node_child(node, current[i]).leaf;
        assert(node);
    }

    passgen_markov_leaf *leaf = node;

    size_t choice = passgen_random_u64_max(random, leaf->total_count);
    for(size_t i = 0; i < leaf->capacity; i++) {
        uint32_t count = passgen_markov_leaf_count(leaf, i);
        if(count) {
            if(choice < count) {
                return passgen_markov_leaf_codepoint(leaf, i);
            } else {
                choice -= count;
            }
        }
    }

    assert(false);

    return 0;
}
