#include "passgen/markov.h"
#include "passgen/assert.h"
#include "passgen/config.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "passgen/util/alloc.h"
#include "passgen/util/clear.h"
#define MIN(a, b)            ((a) < (b) ? (a) : (b))
#define SATURATING_SUB(a, b) ((a) -MIN(a, b))

#define MARKOV_LEAF_INITIAL    4
#define MARKOV_LEAF_MULTIPLIER 2

#define passgen_markov_leaf_count_raw(leaf, codepoint) \
    leaf->entries[leaf->capacity].count[codepoint % leaf->capacity]

#define passgen_markov_leaf_codepoint_raw(leaf, index) \
    leaf->entries[0].codepoint[index % leaf->capacity]

/// Sizes for the leaf and node hash maps. These are all prime numbers, with the
/// property that every number is greater than double the previous one.
/// Generated with Ruby with:
///
/// ```ruby
/// require 'prime'
/// p Prime.take(10000).inject([3]){|l, p| l << p if (2*l.last) < p; l}
/// ```
static const size_t markov_sizes[] = {
    3,        7,        17,       37,     79,      163,     331,
    673,      1361,     2729,     5471,   10949,   21911,   43853,
    87719,    175447,   350899,   701819, 1403641, 2807303, 5614657,
    11229331, 22458671, 44917381, 0};

size_t passgen_markov_node_size(size_t capacity) {
    // we use capacity + 1 there to make sure it's a round number, such that the
    // child pointers are not misaligned.
    return sizeof(passgen_markov_node) + (capacity + 1) * sizeof(uint32_t) +
           capacity * sizeof(passgen_markov_node *);
}

size_t passgen_markov_leaf_size(size_t capacity) {
    return sizeof(passgen_markov_leaf) + capacity * sizeof(uint32_t) +
           capacity * sizeof(uint32_t);
}

// Finds the index of a capacity in the list of sizes.
static size_t markov_size_index(size_t capacity) {
    size_t size_index = 0;
    while(markov_sizes[size_index] < capacity) {
        size_index++;
    }
    return size_index;
}

passgen_markov_node *passgen_markov_node_new(size_t size_index) {
    size_t capacity = markov_sizes[size_index];
    passgen_assert(capacity);
    passgen_markov_node *node =
        passgen_alloc_zeroed(passgen_markov_node_size(capacity));
    if(!node) {
        return NULL;
    }
    node->capacity = capacity;
    return node;
}

passgen_markov_leaf *passgen_markov_leaf_new(size_t size_index) {
    size_t capacity = markov_sizes[size_index];
    passgen_assert(capacity);
    passgen_markov_leaf *leaf =
        passgen_alloc_zeroed(passgen_markov_leaf_size(capacity));
    if(!leaf) {
        return NULL;
    }
    leaf->capacity = capacity;
    leaf->total_count = 0;
    return leaf;
}

void passgen_markov_leaf_free(passgen_markov_leaf *leaf) {
    size_t size = passgen_markov_leaf_size(leaf->capacity);
    PASSGEN_CLEAR(leaf);
    passgen_free(leaf, size);
}

// Releases a node, but not its children.
static void markov_node_release(passgen_markov_node *node) {
    size_t size = passgen_markov_node_size(node->capacity);
    PASSGEN_CLEAR(node);
    passgen_free(node, size);
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

    markov_node_release(node);
}

void passgen_markov_node_check(passgen_markov_node *node, size_t level) {
    passgen_assert(node->capacity);
    passgen_assert(node->capacity % 2);
    for(size_t i = 0; i < node->capacity; i++) {
        void *data = passgen_markov_node_child(node, i).node;
        uint32_t codepoint = passgen_markov_node_codepoint(node, i);
        if(codepoint) {
            passgen_assert(data);
        }
        if(data) {
            passgen_assert((codepoint % node->capacity) == i);
            if(level > 2) {
                passgen_markov_node_check(data, level - 1);
            }
        }
    }
}

void passgen_markov_check(passgen_markov *markov) {
    if(markov->root) {
        passgen_markov_node_check(markov->root, markov->level);
    }
}

// Initialize new markov chain with a given level. The root node is only
// allocated once something is added, so this cannot fail.
void passgen_markov_init(passgen_markov *markov, uint8_t level) {
    passgen_assert(level);
    memset(markov, 0, sizeof(*markov));
    markov->level = level;
    markov->root = NULL;
}

// Moves the entries of a leaf into the next larger one in which none of them
// collide, and there is room for the codepoint. Returns NULL if there is no
// memory for it, the leaf is unchanged then.
static passgen_markov_leaf *
markov_leaf_grow(passgen_markov_leaf *leaf, uint32_t codepoint) {
    size_t size_index = markov_size_index(leaf->capacity) + 1;
    for(; markov_sizes[size_index]; size_index++) {
        passgen_markov_leaf *new = passgen_markov_leaf_new(size_index);
        if(!new) {
            return NULL;
        }

        bool collides = false;
        for(size_t i = 0; !collides && i < leaf->capacity; i++) {
            uint32_t count = passgen_markov_leaf_count_raw(leaf, i);
            if(!count) {
                continue;
            }
            uint32_t entry = passgen_markov_leaf_codepoint_raw(leaf, i);
            if(passgen_markov_leaf_count_raw(new, entry)) {
                collides = true;
            } else {
                passgen_markov_leaf_codepoint_raw(new, entry) = entry;
                passgen_markov_leaf_count_raw(new, entry) = count;
            }
        }
        if(passgen_markov_leaf_count_raw(new, codepoint)) {
            collides = true;
        }

        if(!collides) {
            new->total_count = leaf->total_count;
            passgen_markov_leaf_free(leaf);
            return new;
        }

        passgen_markov_leaf_free(new);
    }

    return NULL;
}

passgen_markov_leaf *passgen_markov_leaf_insert(
    passgen_markov_leaf *leaf,
    uint32_t codepoint,
    size_t weight) {
    // don't do anything if asked to insert a codepoint with an empty weight.
    if(weight == 0) {
        return leaf;
    }

    if(passgen_markov_leaf_count_raw(leaf, codepoint) &&
       passgen_markov_leaf_codepoint_raw(leaf, codepoint) != codepoint) {
        leaf = markov_leaf_grow(leaf, codepoint);
        if(!leaf) {
            return NULL;
        }
    }

    // set codepoint
    passgen_markov_leaf_codepoint_raw(leaf, codepoint) = codepoint;
    passgen_markov_leaf_count_raw(leaf, codepoint) += weight;
    leaf->total_count += weight;

    return leaf;
}

// Moves the children of a node into the next larger one in which none of
// them collide, and there is room for the codepoint. Returns NULL if there is
// no memory for it, the node is unchanged then.
static passgen_markov_node *
markov_node_grow(passgen_markov_node *node, uint32_t codepoint) {
    size_t size_index = markov_size_index(node->capacity) + 1;
    for(; markov_sizes[size_index]; size_index++) {
        passgen_markov_node *new = passgen_markov_node_new(size_index);
        if(!new) {
            return NULL;
        }

        bool collides = false;
        for(size_t i = 0; !collides && i < node->capacity; i++) {
            passgen_markov_node *child = passgen_markov_node_child(node, i).node;
            if(!child) {
                continue;
            }
            uint32_t entry = passgen_markov_node_codepoint(node, i);
            if(passgen_markov_node_child(new, entry).node) {
                collides = true;
            } else {
                passgen_markov_node_codepoint(new, entry) = entry;
                passgen_markov_node_child(new, entry).node = child;
            }
        }
        if(passgen_markov_node_child(new, codepoint).node) {
            collides = true;
        }

        // the children now belong to the new node, or still to the old one.
        if(!collides) {
            markov_node_release(node);
            return new;
        }

        markov_node_release(new);
    }

    return NULL;
}

passgen_markov_node *
passgen_markov_node_insert(passgen_markov_node *node, uint32_t codepoint) {
    if(passgen_markov_node_child(node, codepoint).node &&
       passgen_markov_node_codepoint(node, codepoint) != codepoint) {
        node = markov_node_grow(node, codepoint);
        if(!node) {
            return NULL;
        }
    }

    // set codepoint
    passgen_markov_node_codepoint(node, codepoint) = codepoint;

    return node;
}

// Inserts the last codepoint of a sequence into the leaf below a node.
static passgen_status markov_node_insert_leaf(
    passgen_markov_node *node,
    uint32_t codepoint,
    uint32_t last,
    size_t weight) {
    passgen_markov_leaf *leaf = passgen_markov_node_child(node, codepoint).leaf;
    if(!leaf) {
        leaf = passgen_markov_leaf_new(0);
        if(!leaf) {
            return PASSGEN_ERROR_OUT_OF_MEMORY;
        }
        passgen_markov_node_child(node, codepoint).leaf = leaf;
    }

    leaf = passgen_markov_leaf_insert(leaf, last, weight);
    if(!leaf) {
        return PASSGEN_ERROR_OUT_OF_MEMORY;
    }
    passgen_markov_node_child(node, codepoint).leaf = leaf;

    return PASSGEN_OK;
}

passgen_status passgen_markov_node_insert_word(
    passgen_markov_node **node,
    const uint32_t *sequence,
    size_t length,
    size_t weight) {
    if(!*node) {
        *node = passgen_markov_node_new(0);
        if(!*node) {
            return PASSGEN_ERROR_OUT_OF_MEMORY;
        }
    }

    uint32_t codepoint = sequence[0];
    passgen_markov_node *grown = passgen_markov_node_insert(*node, codepoint);
    if(!grown) {
        return PASSGEN_ERROR_OUT_OF_MEMORY;
    }
    *node = grown;

    passgen_status status;
    if(length == 2) {
        status = markov_node_insert_leaf(grown, codepoint, sequence[1], weight);
    } else {
        status = passgen_markov_node_insert_word(
            &passgen_markov_node_child(grown, codepoint).node,
            &sequence[1],
            length - 1,
            weight);
    }

    // a codepoint without a child is not in the node.
    if(!passgen_markov_node_child(grown, codepoint).node) {
        passgen_markov_node_codepoint(grown, codepoint) = 0;
    }

    return status;
}

static passgen_status passgen_markov_insert(
    passgen_markov *markov,
    const uint32_t *sequence,
    size_t weight) {
    passgen_status status = passgen_markov_node_insert_word(
        &markov->root,
        sequence,
        markov->level + 1,
        weight);
    if(status == PASSGEN_OK) {
        markov->count += weight;
    }
    return status;
}

// Add a word to a markov chain
passgen_status passgen_markov_add(
    passgen_markov *markov,
    const uint32_t *word,
    size_t word_len,
    size_t weight) {
    passgen_status status = PASSGEN_OK;

    // insert start sequences
    size_t sequence_len = 2 * markov->level + 1;
    uint32_t sequence[sequence_len];
    memset(sequence, 0, sizeof(sequence[0]) * sequence_len);
    memcpy(
        &sequence[markov->level],
        word,
        sizeof(uint32_t) * MIN(markov->level, word_len));
    for(size_t i = 0; status == PASSGEN_OK && i < MIN(markov->level, word_len);
        i++) {
        status = passgen_markov_insert(markov, &sequence[i], weight);
    }
    for(size_t i = 0;
        status == PASSGEN_OK && i < SATURATING_SUB(word_len, markov->level);
        i++) {
        status = passgen_markov_insert(markov, &word[i], weight);
    }
    if(status != PASSGEN_OK) {
        return status;
    }
    memset(sequence, 0, sizeof(sequence[0]) * sequence_len);
    memcpy(
        &sequence[SATURATING_SUB(markov->level, word_len)],
        &word[SATURATING_SUB(word_len, markov->level)],
        sizeof(uint32_t) * MIN(markov->level, word_len));
    return passgen_markov_insert(markov, &sequence[0], weight);
}

// Free a markov chain
void passgen_markov_free(passgen_markov *markov) {
    if(markov->root) {
        passgen_markov_node_free(markov->root, markov->level);
    }
    PASSGEN_CLEAR(markov);
}

uint32_t passgen_markov_generate(
    passgen_markov *markov,
    const uint32_t *current,
    passgen_random *random,
    double *entropy) {
    passgen_markov_node *node = markov->root;

    for(size_t i = 0; i < markov->level; i++) {
        node =
            (passgen_markov_node *) passgen_markov_node_child(node, current[i])
                .leaf;
        passgen_assert(node);
    }

    passgen_markov_leaf *leaf = (passgen_markov_leaf *) node;

    // record total choices.
    if(entropy) {
        *entropy += log2((double) leaf->total_count);
    }

    size_t choice = passgen_random_u64_max(random, leaf->total_count);
    for(size_t i = 0; i < leaf->capacity; i++) {
        uint32_t count = passgen_markov_leaf_count_raw(leaf, i);
        if(count) {
            if(choice < count) {
                // record bucket (reduces total choices).
                if(entropy) {
                    *entropy -= log2((double) count);
                }

                return passgen_markov_leaf_codepoint_raw(leaf, i);
            } else {
                choice -= count;
            }
        }
    }

    passgen_assert(false);

    return 0;
}

uint32_t
passgen_markov_leaf_count(passgen_markov_leaf *leaf, uint32_t codepoint) {
    if(passgen_markov_leaf_codepoint_raw(leaf, codepoint) == codepoint) {
        return passgen_markov_leaf_count_raw(leaf, codepoint);
    } else {
        return 0;
    }
}
