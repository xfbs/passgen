#include "passgen/markov.h"
#include "tests.h"
#include <stdlib.h>

#define SEED 234720984723

test_result test_markov_node_new(void) {
    passgen_markov_node *node = passgen_markov_node_new(8, false);
    assert(node);
    assert_eq(node->capacity, 8);

    uint32_t *codepoint;
    size_t *cumulative;
    passgen_markov_node **child;

    for(size_t i = 0; i < node->capacity; i++) {
        cumulative = passgen_markov_node_cumulative(node, i);
        assert_eq(*cumulative, 0);
        *cumulative = i + 1;
    }

    for(size_t i = 0; i < node->capacity; i++) {
        codepoint = passgen_markov_node_codepoint(node, i);
        assert_eq(*codepoint, 0);
        *codepoint = i + 1;
    }

    for(size_t i = 0; i < node->capacity; i++) {
        child = passgen_markov_node_child(node, i);
        assert_eq(*child, 0);
        *child = node;
    }

    free(node);

    return test_ok;
}

test_result test_markov_node_resize(void) {
    passgen_markov_node *node = passgen_markov_node_new(8, false);
    assert(node);
    assert_eq(node->capacity, 8);

    uint32_t *codepoint;
    size_t *cumulative;
    passgen_markov_node **child;

    for(size_t i = 0; i < node->capacity; i++) {
        cumulative = passgen_markov_node_cumulative(node, i);
        assert_eq(*cumulative, 0);
        *cumulative = i + 1;
        codepoint = passgen_markov_node_codepoint(node, i);
        assert_eq(*codepoint, 0);
        *codepoint = i + 1;
        child = passgen_markov_node_child(node, i);
        assert_eq(*child, 0);
        *child = node;
    }

    passgen_markov_node *prev = node;
    node = passgen_markov_node_resize(node, 16, false);
    assert(node);
    assert_eq(node->capacity, 16);

    for(size_t i = 0; i < 8; i++) {
        cumulative = passgen_markov_node_cumulative(node, i);
        assert_eq(*cumulative, i + 1);
        codepoint = passgen_markov_node_codepoint(node, i);
        assert_eq(*codepoint, i + 1);
        child = passgen_markov_node_child(node, i);
        assert_eq(*child, prev);
    }

    for(size_t i = 8; i < node->capacity; i++) {
        cumulative = passgen_markov_node_cumulative(node, i);
        assert_eq(*cumulative, 0);
        codepoint = passgen_markov_node_codepoint(node, i);
        assert_eq(*codepoint, 0);
        child = passgen_markov_node_child(node, i);
        assert_eq(*child, 0);
    }

    free(node);

    return test_ok;
}

test_result test_markov_add(void) {
    passgen_markov markov;
    passgen_markov_init(&markov, 1);

    passgen_markov_add(&markov, &(const uint32_t[]){'a'}, 1, 1);
    passgen_markov_add(&markov, &(const uint32_t[]){'c'}, 1, 1);
    passgen_markov_add(&markov, &(const uint32_t[]){'b'}, 1, 1);

    passgen_markov_free(&markov);

    passgen_markov_init(&markov, 2);

    passgen_markov_add(&markov, &(const uint32_t[]){'a', 'b'}, 2, 1);
    passgen_markov_add(&markov, &(const uint32_t[]){'b'}, 1, 1);
    passgen_markov_add(&markov, &(const uint32_t[]){'c', 'd', 'e', 'f'}, 4, 1);

    passgen_markov_free(&markov);

    return test_ok;
}

test_result test_markov_generate(void) {
    passgen_random_t *rand = passgen_random_new_xorshift(SEED);
    //passgen_random_open_xorshift(&rand, SEED);

    passgen_markov markov;
    passgen_markov_init(&markov, 2);

    passgen_markov_add(&markov, &(const uint32_t[]){'a', 'b'}, 2, 1);
    passgen_markov_add(&markov, &(const uint32_t[]){'b'}, 1, 1);
    passgen_markov_add(&markov, &(const uint32_t[]){'c', 'd', 'e', 'f'}, 4, 1);

    uint32_t next, word[32];
    memset(word, 0, sizeof(word));

    word[2] = passgen_markov_generate(&markov, &word[0], rand);
    assert(word[2] == 'c');
    word[3] = passgen_markov_generate(&markov, &word[1], rand);
    assert(word[3] == 'd');
    word[4] = passgen_markov_generate(&markov, &word[2], rand);
    assert(word[4] == 'e');
    word[5] = passgen_markov_generate(&markov, &word[3], rand);
    assert(word[5] == 'f');
    word[6] = passgen_markov_generate(&markov, &word[4], rand);
    assert(word[6] == 0);

    word[2] = passgen_markov_generate(&markov, &word[0], rand);
    assert(word[2] == 'b');
    word[3] = passgen_markov_generate(&markov, &word[1], rand);
    assert(word[3] == 0);

    passgen_markov_free(&markov);
    passgen_random_free(rand);
}
