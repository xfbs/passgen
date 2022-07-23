#include "passgen/markov.h"
#include "tests.h"
#include <stdlib.h>

#define SEED 234720984723

test_result test_markov_node_layout(void) {
    passgen_markov_node *node = passgen_markov_node_new(0);

    assert_eq(node->capacity, 3);

    assert_eq(&passgen_markov_node_codepoint(node, 0), ((void *) node) + sizeof(size_t));
    assert_eq(&passgen_markov_node_codepoint(node, 1), ((void *) node) + sizeof(size_t) + 1 * sizeof(uint32_t));
    assert_eq(&passgen_markov_node_codepoint(node, 2), ((void *) node) + sizeof(size_t) + 2 * sizeof(uint32_t));

    assert_eq(&passgen_markov_node_child(node, 0), ((void *) node) + sizeof(size_t) + 4 * sizeof(uint32_t));
    assert_eq(&passgen_markov_node_child(node, 1), ((void *) node) + 2 * sizeof(size_t) + 4 * sizeof(uint32_t));
    assert_eq(&passgen_markov_node_child(node, 2), ((void *) node) + 3 * sizeof(size_t) + 4 * sizeof(uint32_t));

    free(node);

    return test_ok;
}

test_result test_markov_leaf_layout(void) {
    passgen_markov_leaf *leaf = passgen_markov_leaf_new(0);

    assert_eq(leaf->capacity, 3);

    assert_eq(&passgen_markov_leaf_codepoint(leaf, 0), ((void *) leaf) + 2 * sizeof(size_t));
    assert_eq(&passgen_markov_leaf_codepoint(leaf, 1), ((void *) leaf) + 2 * sizeof(size_t) + 1 * sizeof(uint32_t));
    assert_eq(&passgen_markov_leaf_codepoint(leaf, 2), ((void *) leaf) + 2 * sizeof(size_t) + 2 * sizeof(uint32_t));

    assert_eq(&passgen_markov_leaf_count(leaf, 0), ((void *) leaf) + 2 * sizeof(size_t) + 3 * sizeof(uint32_t));
    assert_eq(&passgen_markov_leaf_count(leaf, 1), ((void *) leaf) + 2 * sizeof(size_t) + 4 * sizeof(uint32_t));
    assert_eq(&passgen_markov_leaf_count(leaf, 2), ((void *) leaf) + 2 * sizeof(size_t) + 5 * sizeof(uint32_t));

    free(leaf);

    return test_ok;
}

test_result test_markov_node_insert(void) {
    passgen_markov_node *node = passgen_markov_node_new(0);
    assert_eq(node->capacity, 3);

    node = passgen_markov_node_insert(node, 0);
    assert(!passgen_markov_node_child(node, 0).node);
    passgen_markov_node_child(node, 0).node = (passgen_markov_node *) &node;

    node = passgen_markov_node_insert(node, 1);
    assert(!passgen_markov_node_child(node, 1).node);
    passgen_markov_node_child(node, 1).node = (passgen_markov_node *) &node;

    node = passgen_markov_node_insert(node, 2);
    assert(!passgen_markov_node_child(node, 2).node);
    passgen_markov_node_child(node, 2).node = (passgen_markov_node *) &node;

    assert_eq(node->capacity, 3);

    node = passgen_markov_node_insert(node, 3);
    assert(!passgen_markov_node_child(node, 3).node);
    passgen_markov_node_child(node, 3).node = (passgen_markov_node *) &node;

    assert_eq(node->capacity, 7);

    node = passgen_markov_node_insert(node, 4);
    assert(!passgen_markov_node_child(node, 4).node);
    passgen_markov_node_child(node, 4).node = (passgen_markov_node *) &node;

    node = passgen_markov_node_insert(node, 5);
    assert(!passgen_markov_node_child(node, 5).node);
    passgen_markov_node_child(node, 5).node = (passgen_markov_node *) &node;

    node = passgen_markov_node_insert(node, 6);
    assert(!passgen_markov_node_child(node, 6).node);
    passgen_markov_node_child(node, 6).node = (passgen_markov_node *) &node;

    node = passgen_markov_node_insert(node, 7);
    assert(!passgen_markov_node_child(node, 7).node);
    passgen_markov_node_child(node, 7).node = (passgen_markov_node *) &node;

    assert_eq(node->capacity, 17);

    for(size_t i = 8; i < 1000; i++) {
        node = passgen_markov_node_insert(node, i);
        assert(!passgen_markov_node_child(node, i).node);
        passgen_markov_node_child(node, i).node = (passgen_markov_node *) &node;
    }

    assert_eq(node->capacity, 1361);

    free(node);

    return test_ok;
}

test_result test_markov_leaf_insert(void) {
    passgen_markov_leaf *leaf = passgen_markov_leaf_new(0);
    assert_eq(leaf->capacity, 3);

    leaf = passgen_markov_leaf_insert(leaf, 0, 1);
    assert(passgen_markov_leaf_count(leaf, 0) == 1);

    leaf = passgen_markov_leaf_insert(leaf, 0, 1);
    assert(passgen_markov_leaf_count(leaf, 0) == 2);

    leaf = passgen_markov_leaf_insert(leaf, 1, 3);
    assert(passgen_markov_leaf_count(leaf, 1) == 3);

    leaf = passgen_markov_leaf_insert(leaf, 2, 7);
    assert(passgen_markov_leaf_count(leaf, 2) == 7);

    assert_eq(leaf->capacity, 3);

    leaf = passgen_markov_leaf_insert(leaf, 3, 2);
    assert(passgen_markov_leaf_count(leaf, 3) == 2);

    assert_eq(leaf->capacity, 7);

    leaf = passgen_markov_leaf_insert(leaf, 4, 4);
    assert(passgen_markov_leaf_count(leaf, 4) == 4);

    leaf = passgen_markov_leaf_insert(leaf, 5, 6);
    assert(passgen_markov_leaf_count(leaf, 5) == 6);

    leaf = passgen_markov_leaf_insert(leaf, 6, 8);
    assert(passgen_markov_leaf_count(leaf, 6) == 8);

    leaf = passgen_markov_leaf_insert(leaf, 7, 10);
    assert(passgen_markov_leaf_count(leaf, 7) == 10);

    assert_eq(leaf->capacity, 17);

    for(size_t i = 8; i < 1000; i++) {
        leaf = passgen_markov_leaf_insert(leaf, i, i);
        assert(passgen_markov_leaf_count(leaf, i) == i);
    }

    assert_eq(leaf->capacity, 1361);

    free(leaf);

    return test_ok;
}

test_result test_markov_node_insert_word(void) {
    passgen_markov_node *root_node = passgen_markov_node_new(0);

    const uint32_t word[] = {'h', 'e', 'l', 'l', 'o'};

    root_node = passgen_markov_node_insert_word(root_node, (void *) &word, 5, 1);
    passgen_markov_node *node = root_node;

    assert_eq(passgen_markov_node_codepoint(node, 'h'), 'h');
    node = passgen_markov_node_child(node, 'h').node;
    assert(node);

    assert_eq(passgen_markov_node_codepoint(node, 'e'), 'e');
    node = passgen_markov_node_child(node, 'e').node;
    assert(node);

    assert_eq(passgen_markov_node_codepoint(node, 'l'), 'l');
    node = passgen_markov_node_child(node, 'l').node;
    assert(node);

    assert_eq(passgen_markov_node_codepoint(node, 'l'), 'l');
    passgen_markov_leaf *leaf = passgen_markov_node_child(node, 'l').leaf;
    assert(leaf);
    assert_eq(leaf->capacity, 3);
    assert_eq(leaf->total_count, 1);

    assert_eq(passgen_markov_leaf_codepoint(leaf, 'o'), 'o');
    assert_eq(passgen_markov_leaf_count(leaf, 'o'), 1);

    passgen_markov_node_free(root_node, 4);

    return test_ok;
}

test_result test_markov_add(void) {
    passgen_markov markov;
    passgen_markov_node *node;
    passgen_markov_leaf *leaf;

    // markov chain level 2, meaning two prefix chars
    passgen_markov_init(&markov, 2);

    // this should have added 00a and 0a0 into the chain.
    passgen_markov_add(&markov, (void *) &(const uint32_t[]){'a'}, 1, 1);

    // verify 00a is there
    node = passgen_markov_node_child(markov.root, 0).node;
    assert(node);
    leaf = passgen_markov_node_child(node, 0).leaf;
    assert(leaf);
    assert_eq(leaf->total_count, 1);
    assert_eq(passgen_markov_leaf_codepoint(leaf, 'a'), 'a');
    assert_eq(passgen_markov_leaf_count(leaf, 'a'), 1);

    // verify 0a0 is there
    node = passgen_markov_node_child(markov.root, 0).node;
    assert(node);
    leaf = passgen_markov_node_child(node, 'a').leaf;
    assert(leaf);
    assert_eq(leaf->total_count, 1);
    assert_eq(passgen_markov_leaf_codepoint(leaf, 0), 0);
    assert_eq(passgen_markov_leaf_count(leaf, 0), 1);

    // this should have added 00l, 0la, la0, each with a weight of 2.
    passgen_markov_add(&markov, (void *) &(const uint32_t[]){'l', 'a'}, 2, 2);

    // verify 00l is there
    node = passgen_markov_node_child(markov.root, 0).node;
    assert(node);
    leaf = passgen_markov_node_child(node, 0).leaf;
    assert(leaf);
    assert_eq(leaf->total_count, 3);
    assert_eq(passgen_markov_leaf_codepoint(leaf, 'l'), 'l');
    assert_eq(passgen_markov_leaf_count(leaf, 'l'), 2);

    // verify 0la is there
    node = passgen_markov_node_child(markov.root, 0).node;
    assert(node);
    leaf = passgen_markov_node_child(node, 'l').leaf;
    assert(leaf);
    assert_eq(leaf->total_count, 2);
    assert_eq(passgen_markov_leaf_codepoint(leaf, 'a'), 'a');
    assert_eq(passgen_markov_leaf_count(leaf, 'a'), 2);

    // verify la0 is there
    node = passgen_markov_node_child(markov.root, 'l').node;
    assert(node);
    leaf = passgen_markov_node_child(node, 'a').leaf;
    assert(leaf);
    assert_eq(leaf->total_count, 2);
    assert_eq(passgen_markov_leaf_codepoint(leaf, 0), 0);
    assert_eq(passgen_markov_leaf_count(leaf, 0), 2);

    passgen_markov_add(&markov, (void *) &(const uint32_t[]){'l', 'e'}, 2, 1);
    passgen_markov_add(&markov, (void *) &(const uint32_t[]){'t', 'h', 'e'}, 3, 1);
    passgen_markov_add(&markov, (void *) &(const uint32_t[]){'p', 'a', 'r', 't'}, 4, 1);
    passgen_markov_add(&markov, (void *) &(const uint32_t[]){'p', 'h', 'o', 'n', 'e'}, 5, 1);

    passgen_markov_free(&markov);

    return test_ok;
}

test_result test_markov_generate(void) {
    /*
    passgen_random *rand = passgen_random_new_xorshift(SEED);
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
    */

    return test_ok;
}
