#include "passgen/markov.h"
#include "passgen/assert.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define SATURATING_SUB(a, b) ((a) - MIN(a, b))

#define MARKOV_LIST_INITIAL    4
#define MARKOV_LIST_MULTIPLIER 2

size_t passgen_markov_node_size(size_t capacity, bool leaf) {
    size_t item_size = sizeof(size_t) + sizeof(uint32_t) + (leaf ? 0 : sizeof(passgen_markov_node *));
    size_t size = sizeof(passgen_markov_node) + capacity * item_size;
    return size;
}

passgen_markov_node *passgen_markov_node_new(size_t capacity, bool leaf) {
    size_t size = passgen_markov_node_size(capacity, leaf);
    passgen_markov_node *node = calloc(1, size);
    node->capacity = capacity;
    return node;
}

void passgen_markov_node_free(passgen_markov_node *node, size_t level) {
    if(level) {
        for(size_t i = 0; i < node->capacity; i++) {
            size_t *cumulative = passgen_markov_node_cumulative(node, i);
            if(*cumulative) {
                passgen_markov_node **child = passgen_markov_node_child(node, i);
                passgen_markov_node_free(*child, level - 1);
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
    markov->root = passgen_markov_node_new(MARKOV_LIST_INITIAL, false);
}

uint32_t *passgen_markov_node_codepoint(passgen_markov_node *node, size_t index) {
    return &node->data[node->capacity].codepoint[index];
}

size_t *passgen_markov_node_cumulative(passgen_markov_node *node, size_t index) {
    return &node->data[0].cumulative[index];
}

passgen_markov_node **passgen_markov_node_child(passgen_markov_node *node, size_t index) {
    return &node->data[node->capacity + node->capacity / 2].child[index];
}

size_t passgen_markov_node_find(
    passgen_markov_node *node,
    uint32_t codepoint) {
    for(size_t i = 0; i < node->capacity; i++) {
        uint32_t current = *passgen_markov_node_codepoint(node, i);
        if(!current && (i || !*passgen_markov_node_cumulative(node, i))) {
            return i;
        }
        if(current >= codepoint) {
            return i;
        }
    }

    return node->capacity;
}

passgen_markov_node *passgen_markov_node_resize(passgen_markov_node *node, size_t capacity, bool leaf) {
    if(capacity > node->capacity) {
        passgen_markov_node *new_node = passgen_markov_node_new(capacity, leaf);

        size_t *old_cumulative = passgen_markov_node_cumulative(node, 0);
        uint32_t *old_codepoint = passgen_markov_node_codepoint(node, 0);
        passgen_markov_node **old_child = passgen_markov_node_child(node, 0);

        size_t *new_cumulative = passgen_markov_node_cumulative(new_node, 0);
        uint32_t *new_codepoint = passgen_markov_node_codepoint(new_node, 0);
        passgen_markov_node **new_child = passgen_markov_node_child(new_node, 0);

        memcpy(new_cumulative, old_cumulative, node->capacity * sizeof(size_t));
        memcpy(new_codepoint, old_codepoint, node->capacity * sizeof(uint32_t));
        if(!leaf) {
            memcpy(new_child, old_child, node->capacity * sizeof(passgen_markov_node *));
        }

        free(node);
        return new_node;
    } else {
        return NULL;
    }
}

void passgen_markov_node_move(passgen_markov_node *node, size_t position, bool leaf) {
    size_t amount = node->capacity - position - 1;
    uint32_t *codepoint = passgen_markov_node_codepoint(node, position);
    size_t *cumulative = passgen_markov_node_cumulative(node, position);
    passgen_markov_node **child = passgen_markov_node_child(node, position);
    memmove(codepoint + 1, codepoint, amount * sizeof(uint32_t));
    memmove(cumulative + 1, cumulative, amount * sizeof(size_t));
    if(!leaf) {
        memmove(child + 1, child, amount * sizeof(passgen_markov_node *));
    }
    *codepoint = 0;
    *cumulative = 0;
    if(!leaf) {
        *child = NULL;
    }
}

passgen_markov_node *passgen_markov_node_insert(
    passgen_markov_node *node,
    const uint32_t *sequence,
    size_t length,
    size_t weight) {
    size_t pos = passgen_markov_node_find(node, sequence[0]);

    if(pos == node->capacity || *passgen_markov_node_cumulative(node, node->capacity - 1)) {
        node = passgen_markov_node_resize(node, MARKOV_LIST_MULTIPLIER * node->capacity, length == 1);
    }

    uint32_t *codepoint = passgen_markov_node_codepoint(node, pos);
    size_t *cumulative = passgen_markov_node_cumulative(node, pos);
    passgen_markov_node **child = passgen_markov_node_child(node, pos);
    if(!*cumulative) {
        *codepoint = sequence[0];
        *cumulative = 0;
        if(length > 1) {
            passgen_assert(!*child);
            *child = passgen_markov_node_new(MARKOV_LIST_INITIAL, length == 2);
        }
    }
    if(*codepoint != sequence[0]) {
        passgen_markov_node_move(node, pos, length == 1);
        *codepoint = sequence[0];
        *cumulative = 0;
        if(length > 1) {
            passgen_assert(!*child);
            *child = passgen_markov_node_new(MARKOV_LIST_INITIAL, length == 2);
        }
    }
    *cumulative += weight;
    if(length > 1) {
        *child = passgen_markov_node_insert(*child, &sequence[1], length - 1, weight);
    }

    return node;
}

void passgen_markov_insert(
    passgen_markov *markov,
    const uint32_t *sequence,
    size_t weight) {
    markov->root = passgen_markov_node_insert(markov->root, sequence, markov->level + 1, weight);
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
        size_t pos = passgen_markov_node_find(node, current[i]);
        passgen_assert(*passgen_markov_node_codepoint(node, pos) == current[i]);
        choices = *passgen_markov_node_cumulative(node, pos);
        node = *passgen_markov_node_child(node, pos);
    }

    passgen_assert(choices);
    passgen_assert(node);

    size_t choice = passgen_random_u64_max(random, choices);
    for(size_t i = 0; i < node->capacity; i++) {
        size_t cumulative = *passgen_markov_node_cumulative(node, i);
        if(cumulative > choice) {
            return *passgen_markov_node_codepoint(node, i);
        } else {
            choice -= cumulative;
        }
    }

    return 0;
}
