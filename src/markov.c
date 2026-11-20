#include "passgen/markov.h"
#include "passgen/assert.h"
#include "passgen/config.h"
#include "passgen/util/alloc.h"
#include "passgen/util/clear.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define MIN(a, b)            ((a) < (b) ? (a) : (b))
#define SATURATING_SUB(a, b) ((a) - MIN(a, b))

// Size of a node with the given number of children, which are followed by
// their codepoints.
static size_t markov_node_size(size_t len) {
    return sizeof(passgen_markov_node) +
           len * (sizeof(passgen_markov_child) + sizeof(uint32_t));
}

// Size of a leaf with the given number of codepoints.
static size_t markov_leaf_size(size_t len) {
    return sizeof(passgen_markov_leaf) + len * sizeof(passgen_markov_frequency);
}

// How many steps the search takes from its guess, before it goes on with a
// binary search.
#ifndef MARKOV_SEARCH_STEPS
#define MARKOV_SEARCH_STEPS 8
#endif

// Finds where a codepoint is in a sorted list of them, or where it would go
// to keep them sorted. Sets found to whether it is there.
//
// The codepoints of a list are mostly runs of letters which follow each
// other, such as `a` to `z`. So where a codepoint falls between the first and
// the last one is a good guess for where it is, from which it is mostly
// found in a few steps. A binary search takes more, with branches which are
// hard to predict, and is slower for the lists which building a Markov chain
// looks at most, of a few dozen codepoints.
//
// For lists where the guess is far off (such as letters and a single emoji),
// it goes on with a binary search after a few steps, so that it is never
// much slower than one. It is inlined, so that the stride is known.
static inline size_t markov_search(
    const uint32_t *codepoints,
    size_t stride,
    size_t len,
    uint32_t codepoint,
    bool *found) {
#define AT(i) codepoints[(i) * stride]
    *found = false;
    if(len == 0) {
        return 0;
    }

    // most lists start with the zero which ends words, which is far from the
    // letters, so the guess starts from the one after it.
    size_t low = AT(0) == 0 && codepoint != 0 && len > 1;
    size_t high = len - 1;
    if(codepoint <= AT(low)) {
        *found = codepoint == AT(low);
        return low;
    }
    if(codepoint >= AT(high)) {
        *found = codepoint == AT(high);
        return codepoint == AT(high) ? high : len;
    }

    // from here on, the codepoint at low is smaller and the one at high is
    // larger than the one which is looked for, so it goes between them if it
    // is not there. the guess is between them as well.
    size_t index = low + (size_t) ((uint64_t) (codepoint - AT(low)) *
                                   (high - low) / (AT(high) - AT(low)));
    if(index <= low) {
        index = low + 1;
    }
    if(index >= high) {
        index = high - 1;
    }

    if(high - low > 1) {
        uint32_t entry = AT(index);
        if(entry == codepoint) {
            *found = true;
            return index;
        }

        // a few steps towards it.
        if(entry > codepoint) {
            high = index;
            for(int i = 0; i < MARKOV_SEARCH_STEPS && high - low > 1; i++) {
                entry = AT(high - 1);
                if(entry < codepoint) {
                    return high;
                }
                high -= 1;
                if(entry == codepoint) {
                    *found = true;
                    return high;
                }
            }
        } else {
            low = index;
            for(int i = 0; i < MARKOV_SEARCH_STEPS && high - low > 1; i++) {
                entry = AT(low + 1);
                if(entry > codepoint) {
                    return low + 1;
                }
                low += 1;
                if(entry == codepoint) {
                    *found = true;
                    return low;
                }
            }
        }
    }

    // a binary search for the rest.
    while(high - low > 1) {
        size_t middle = low + (high - low) / 2;
        uint32_t entry = AT(middle);
        if(entry == codepoint) {
            *found = true;
            return middle;
        }
        if(entry < codepoint) {
            low = middle;
        } else {
            high = middle;
        }
    }

    return high;
#undef AT
}

// Finds a codepoint in a leaf, see markov_search().
static size_t markov_leaf_search(
    const passgen_markov_leaf *leaf,
    uint32_t codepoint,
    bool *found) {
    // the codepoints are every other uint32_t of the entries.
    passgen_static_assert(
        sizeof(passgen_markov_frequency) == 2 * sizeof(uint32_t));
    return markov_search(
        &leaf->entries[0].codepoint,
        2,
        leaf->len,
        codepoint,
        found);
}

// Finds a codepoint in a node, see markov_search().
static size_t markov_node_search(
    const passgen_markov_node *node,
    uint32_t codepoint,
    bool *found) {
    return markov_search(
        passgen_markov_node_codepoints(node),
        1,
        node->len,
        codepoint,
        found);
}

passgen_markov_node *passgen_markov_node_new(void) {
    passgen_markov_node *node = passgen_alloc(markov_node_size(0));
    if(!node) {
        return NULL;
    }
    node->len = 0;
    return node;
}

passgen_markov_leaf *passgen_markov_leaf_new(void) {
    passgen_markov_leaf *leaf = passgen_alloc(markov_leaf_size(0));
    if(!leaf) {
        return NULL;
    }
    leaf->len = 0;
    leaf->total_count = 0;
    return leaf;
}

void passgen_markov_leaf_free(passgen_markov_leaf *leaf) {
    size_t size = markov_leaf_size(leaf->len);
    memset(leaf, 0, size);
    passgen_free(leaf, size);
}

// Releases a node, but not its children.
static void markov_node_release(passgen_markov_node *node) {
    size_t size = markov_node_size(node->len);
    memset(node, 0, size);
    passgen_free(node, size);
}

// Frees what a node points to, which is a node with the given level, or a
// leaf for a level of zero.
static void markov_child_free(passgen_markov_child child, size_t level) {
    if(level == 0) {
        passgen_markov_leaf_free(child.leaf);
    } else {
        passgen_markov_node_free(child.node, level);
    }
}

void passgen_markov_node_free(passgen_markov_node *node, size_t level) {
    for(size_t i = 0; i < node->len; i++) {
        markov_child_free(node->children[i], level - 1);
    }

    markov_node_release(node);
}

passgen_markov_child *
passgen_markov_node_child(const passgen_markov_node *node, uint32_t codepoint) {
    bool found;
    size_t index = markov_node_search(node, codepoint, &found);
    if(!found) {
        return NULL;
    }
    return (passgen_markov_child *) &node->children[index];
}

// Initialize new markov chain with a given level. The root node is only
// allocated once something is added, so this cannot fail.
void passgen_markov_init(passgen_markov *markov, uint8_t level) {
    memset(markov, 0, sizeof(*markov));
    markov->level = level;
    markov->root.node = NULL;
}

passgen_markov_leaf *passgen_markov_leaf_insert(
    passgen_markov_leaf *leaf,
    uint32_t codepoint,
    size_t weight) {
    // don't do anything if asked to insert a codepoint with an empty weight.
    if(weight == 0) {
        return leaf;
    }

    bool found;
    size_t index = markov_leaf_search(leaf, codepoint, &found);

    if(!found) {
        // this grows by a single entry, so that no memory is wasted. most
        // leaves only have a few.
        passgen_markov_leaf *grown = passgen_resize(
            leaf,
            markov_leaf_size(leaf->len),
            markov_leaf_size(leaf->len + 1));
        if(!grown) {
            return NULL;
        }
        leaf = grown;

        memmove(
            &leaf->entries[index + 1],
            &leaf->entries[index],
            (leaf->len - index) * sizeof(leaf->entries[0]));
        leaf->entries[index].codepoint = codepoint;
        leaf->entries[index].count = 0;
        leaf->len += 1;
    }

    leaf->entries[index].count += weight;
    leaf->total_count += weight;

    return leaf;
}

// Adds a child to a node, at the index where its codepoint goes. Returns the
// node, which may have moved, or NULL if there is no memory for it, the node
// is unchanged then.
static passgen_markov_node *markov_node_insert_at(
    passgen_markov_node *node,
    size_t index,
    uint32_t codepoint,
    passgen_markov_child child) {
    size_t len = node->len;
    passgen_markov_node *grown =
        passgen_resize(node, markov_node_size(len), markov_node_size(len + 1));
    if(!grown) {
        return NULL;
    }
    node = grown;

    // the codepoints follow the children, so they move along by one child
    // first. they are where the new child goes otherwise.
    uint32_t *old_codepoints = (uint32_t *) &node->children[len];
    uint32_t *codepoints = (uint32_t *) &node->children[len + 1];
    memmove(codepoints, old_codepoints, len * sizeof(uint32_t));

    memmove(
        &codepoints[index + 1],
        &codepoints[index],
        (len - index) * sizeof(uint32_t));
    codepoints[index] = codepoint;

    memmove(
        &node->children[index + 1],
        &node->children[index],
        (len - index) * sizeof(node->children[0]));
    node->children[index] = child;

    node->len = len + 1;
    return node;
}

passgen_status passgen_markov_node_insert_word(
    passgen_markov_node **node,
    const uint32_t *sequence,
    size_t length,
    size_t weight) {
    // a word without weight would leave a leaf without any codepoints, which
    // there would be nothing to generate from.
    if(weight == 0) {
        return PASSGEN_OK;
    }

    if(!*node) {
        *node = passgen_markov_node_new();
        if(!*node) {
            return PASSGEN_ERROR_OUT_OF_MEMORY;
        }
    }

    bool found;
    size_t index = markov_node_search(*node, sequence[0], &found);
    passgen_markov_child child = {NULL};
    if(found) {
        child = (*node)->children[index];
    }

    // the levels of nodes below this one, and so of the child.
    size_t level = length - 2;
    passgen_status status = PASSGEN_OK;

    if(level == 0) {
        // the child is the leaf, which counts the last codepoint.
        passgen_markov_leaf *leaf = child.leaf;
        if(!leaf) {
            leaf = passgen_markov_leaf_new();
            if(!leaf) {
                return PASSGEN_ERROR_OUT_OF_MEMORY;
            }
        }

        passgen_markov_leaf *grown =
            passgen_markov_leaf_insert(leaf, sequence[1], weight);
        if(!grown) {
            if(!found) {
                passgen_markov_leaf_free(leaf);
            }
            return PASSGEN_ERROR_OUT_OF_MEMORY;
        }
        child.leaf = grown;
    } else {
        // the child may move when it grows, or be allocated.
        status = passgen_markov_node_insert_word(
            &child.node,
            &sequence[1],
            length - 1,
            weight);
    }

    // a child which was there before is updated, whether this worked or not,
    // as it may have moved.
    if(found) {
        (*node)->children[index] = child;
        return status;
    }

    // a new child which could not be filled in is not added.
    if(status != PASSGEN_OK) {
        if(child.node) {
            markov_child_free(child, level);
        }
        return status;
    }

    passgen_markov_node *grown =
        markov_node_insert_at(*node, index, sequence[0], child);
    if(!grown) {
        markov_child_free(child, level);
        return PASSGEN_ERROR_OUT_OF_MEMORY;
    }
    *node = grown;

    return PASSGEN_OK;
}

static passgen_status passgen_markov_insert(
    passgen_markov *markov,
    const uint32_t *sequence,
    size_t weight) {
    // a chain of length zero only counts the codepoints, in its leaf.
    if(markov->level == 0) {
        if(weight == 0) {
            return PASSGEN_OK;
        }
        passgen_markov_leaf *leaf = markov->root.leaf;
        if(!leaf) {
            leaf = passgen_markov_leaf_new();
            if(!leaf) {
                return PASSGEN_ERROR_OUT_OF_MEMORY;
            }
        }
        passgen_markov_leaf *grown =
            passgen_markov_leaf_insert(leaf, sequence[0], weight);
        if(!grown) {
            if(!markov->root.leaf) {
                passgen_markov_leaf_free(leaf);
            }
            return PASSGEN_ERROR_OUT_OF_MEMORY;
        }
        markov->root.leaf = grown;
        markov->count += weight;
        return PASSGEN_OK;
    }

    passgen_status status = passgen_markov_node_insert_word(
        &markov->root.node,
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
    if(markov->level == 0) {
        if(markov->root.leaf) {
            passgen_markov_leaf_free(markov->root.leaf);
        }
    } else if(markov->root.node) {
        passgen_markov_node_free(markov->root.node, markov->level);
    }
    PASSGEN_CLEAR(markov);
}

uint32_t passgen_markov_generate(
    passgen_markov *markov,
    const uint32_t *current,
    passgen_random *random,
    double *entropy) {
    passgen_markov_leaf *leaf = markov->root.leaf;
    if(markov->level > 0) {
        passgen_markov_node *node = markov->root.node;
        passgen_assert(node);

        // the last of the previous codepoints leads to the leaf.
        for(size_t i = 0; i + 1 < markov->level; i++) {
            passgen_markov_child *child =
                passgen_markov_node_child(node, current[i]);
            passgen_assert(child);
            node = child->node;
        }
        passgen_markov_child *child =
            passgen_markov_node_child(node, current[markov->level - 1]);
        passgen_assert(child);
        leaf = child->leaf;
    }
    passgen_assert(leaf && leaf->len);

    // with only one codepoint which can follow (however often it was seen),
    // there is nothing to choose from, so this takes no randomness. the
    // entropy of that is zero.
    if(leaf->len == 1) {
        return leaf->entries[0].codepoint;
    }

    // the codepoints are chosen from in their order.
    size_t choice = passgen_random_u64_max(random, leaf->total_count);
    for(size_t i = 0; i < leaf->len; i++) {
        uint32_t count = leaf->entries[i].count;
        if(choice < count) {
            // this was one of total_count equally likely choices, of which
            // count result in this codepoint.
            if(entropy) {
                *entropy +=
                    log2((double) leaf->total_count) - log2((double) count);
            }
            return leaf->entries[i].codepoint;
        }
        choice -= count;
    }

    // the counts add up to the total, so one of them was chosen.
    passgen_assert(false);
    return 0;
}

uint32_t
passgen_markov_leaf_count(const passgen_markov_leaf *leaf, uint32_t codepoint) {
    bool found;
    size_t index = markov_leaf_search(leaf, codepoint, &found);
    return found ? leaf->entries[index].count : 0;
}
