#include "passgen/markov.h"
#include "tests.h"
#include <stdlib.h>

#define SEED 234720984723

#define passgen_random_unicode(rand) (passgen_random_u32(rand) & 0x10FFFF)

/// The codepoints of a node follow its children, in the same allocation.
test_result test_markov_node_layout(void) {
    passgen_markov_node *node = NULL;
    const uint32_t words[][2] = {{'c', 'x'}, {'a', 'y'}, {'b', 'z'}};
    for(size_t i = 0; i < 3; i++) {
        assert_eq(
            PASSGEN_OK,
            passgen_markov_node_insert_word(&node, words[i], 2, 1));
    }

    assert_eq(node->len, 3);
    uint32_t *codepoints = passgen_markov_node_codepoints(node);
    assert_eq(
        (void *) codepoints,
        (void *) ((char *) node + sizeof(size_t) + 3 * sizeof(void *)));

    // sorted, whatever order they were added in.
    assert_eq(codepoints[0], 'a');
    assert_eq(codepoints[1], 'b');
    assert_eq(codepoints[2], 'c');
    assert_eq(passgen_markov_leaf_count(node->children[0].leaf, 'y'), 1);
    assert_eq(passgen_markov_leaf_count(node->children[1].leaf, 'z'), 1);
    assert_eq(passgen_markov_leaf_count(node->children[2].leaf, 'x'), 1);
    assert(passgen_markov_node_child(node, 'b') == &node->children[1]);
    assert(passgen_markov_node_child(node, 'd') == NULL);

    passgen_markov_node_free(node, 1);

    return test_ok;
}

/// Leaves keep their codepoints sorted, with no room to spare.
test_result test_markov_leaf_insert(void) {
    passgen_markov_leaf *leaf = passgen_markov_leaf_new();
    assert(leaf);
    assert_eq(leaf->len, 0);

    const uint32_t codepoints[] = {'m', 'a', 'z', 'm', 0, 'a', 0x10FFFF};
    const size_t weights[] = {1, 2, 3, 4, 5, 6, 7};
    for(size_t i = 0; i < 7; i++) {
        leaf = passgen_markov_leaf_insert(leaf, codepoints[i], weights[i]);
        assert(leaf);
    }

    // a weight of zero does not add anything.
    leaf = passgen_markov_leaf_insert(leaf, 'q', 0);

    assert_eq(leaf->len, 5);
    assert_eq(leaf->total_count, 28);
    const passgen_markov_frequency expected[] = {
        {0, 5},
        {'a', 8},
        {'m', 5},
        {'z', 3},
        {0x10FFFF, 7},
    };
    for(size_t i = 0; i < 5; i++) {
        assert_eq(leaf->entries[i].codepoint, expected[i].codepoint);
        assert_eq(leaf->entries[i].count, expected[i].count);
        assert_eq(
            passgen_markov_leaf_count(leaf, expected[i].codepoint),
            expected[i].count);
    }
    assert_eq(passgen_markov_leaf_count(leaf, 'b'), 0);
    assert_eq(passgen_markov_leaf_count(leaf, 'q'), 0);

    passgen_markov_leaf_free(leaf);

    return test_ok;
}

test_result test_markov_node_insert_word(void) {
    passgen_markov_node *root_node = NULL;

    const uint32_t word[] = {'h', 'e', 'l', 'l', 'o'};

    assert_eq(
        passgen_markov_node_insert_word(&root_node, word, 5, 1),
        PASSGEN_OK);
    passgen_markov_node *node = root_node;

    const uint32_t path[] = {'h', 'e', 'l'};
    for(size_t i = 0; i < 3; i++) {
        assert_eq(node->len, 1);
        passgen_markov_child *child = passgen_markov_node_child(node, path[i]);
        assert(child);
        node = child->node;
        assert(node);
    }

    passgen_markov_child *child = passgen_markov_node_child(node, 'l');
    assert(child);
    passgen_markov_leaf *leaf = child->leaf;
    assert_eq(leaf->len, 1);
    assert_eq(leaf->total_count, 1);
    assert_eq(passgen_markov_leaf_count(leaf, 'o'), 1);

    passgen_markov_node_free(root_node, 4);

    return test_ok;
}

/// Finds the leaf for a prefix of two codepoints.
static passgen_markov_leaf *
leaf_of(passgen_markov *markov, uint32_t first, uint32_t second) {
    passgen_markov_child *child = passgen_markov_node_child(markov->root, first);
    if(!child) {
        return NULL;
    }
    child = passgen_markov_node_child(child->node, second);
    return child ? child->leaf : NULL;
}

test_result test_markov_add(void) {
    passgen_markov markov;
    passgen_markov_leaf *leaf;

    // markov chain level 2, meaning two prefix chars
    passgen_markov_init(&markov, 2);

    // this should have added 00a and 0a0 into the chain.
    passgen_markov_add(&markov, (const uint32_t[]){'a'}, 1, 1);

    leaf = leaf_of(&markov, 0, 0);
    assert(leaf);
    assert_eq(leaf->total_count, 1);
    assert_eq(passgen_markov_leaf_count(leaf, 'a'), 1);

    leaf = leaf_of(&markov, 0, 'a');
    assert(leaf);
    assert_eq(leaf->total_count, 1);
    assert_eq(passgen_markov_leaf_count(leaf, 0), 1);

    // this should have added 00l, 0la, la0, each with a weight of 2.
    passgen_markov_add(&markov, (const uint32_t[]){'l', 'a'}, 2, 2);

    leaf = leaf_of(&markov, 0, 0);
    assert(leaf);
    assert_eq(leaf->total_count, 3);
    assert_eq(passgen_markov_leaf_count(leaf, 'l'), 2);

    leaf = leaf_of(&markov, 0, 'l');
    assert(leaf);
    assert_eq(leaf->total_count, 2);
    assert_eq(passgen_markov_leaf_count(leaf, 'a'), 2);

    leaf = leaf_of(&markov, 'l', 'a');
    assert(leaf);
    assert_eq(leaf->total_count, 2);
    assert_eq(passgen_markov_leaf_count(leaf, 0), 2);

    passgen_markov_add(&markov, (const uint32_t[]){'l', 'e'}, 2, 1);
    passgen_markov_add(&markov, (const uint32_t[]){'t', 'h', 'e'}, 3, 1);
    passgen_markov_add(&markov, (const uint32_t[]){'p', 'a', 'r', 't'}, 4, 1);
    passgen_markov_add(
        &markov,
        (const uint32_t[]){'p', 'h', 'o', 'n', 'e'},
        5,
        1);

    // the root has the codepoints which words start with, sorted.
    uint32_t *codepoints = passgen_markov_node_codepoints(markov.root);
    for(size_t i = 1; i < markov.root->len; i++) {
        assert(codepoints[i - 1] < codepoints[i]);
    }

    passgen_markov_free(&markov);

    return test_ok;
}

test_result test_markov_add_random(void) {
    passgen_markov markov;
    passgen_random rand;
    passgen_random_xorshift_open(&rand, SEED);
    uint32_t word[12] = {0};
    size_t word_count = 200;

    // try this for varying markov chain lengths
    for(size_t length = 3; length < 10; length++) {
        passgen_markov_init(&markov, length);

        // insert words
        for(size_t count = 0; count < word_count; count++) {
            // generate random word
            for(size_t offset = 0; offset < 12; offset++) {
                // generate only non-null, valid unicode codepoints.
                do {
                    word[offset] = passgen_random_unicode(&rand);
                } while(word[offset] == 0);
            }

            assert_eq(PASSGEN_OK, passgen_markov_add(&markov, &word[0], 12, 1));
        }

        passgen_markov_free(&markov);
    }

    passgen_random_close(&rand);

    return test_ok;
}

test_result test_markov_generate_random(void) {
    passgen_markov markov;
    passgen_random rand;
    passgen_random_xorshift_open(&rand, SEED);
    uint32_t word[12] = {0};
    size_t word_count = 1000;
    size_t length = 3;

    passgen_markov_init(&markov, length);

    // insert words
    for(size_t count = 0; count < word_count; count++) {
        // generate random word
        for(size_t offset = 0; offset < 12; offset++) {
            do {
                word[offset] = passgen_random_unicode(&rand);
            } while(word[offset] == 0);
        }

        passgen_markov_add(&markov, &word[0], 12, 1);
    }

    // generate word_count words
    for(size_t count = 0; count < word_count; count++) {
        // clear word
        for(size_t i = 0; i < length; i++) {
            word[i] = 0;
        }

        uint32_t next;
        do {
            // generate next codepoint
            next = passgen_markov_generate(&markov, &word[0], &rand, NULL);

            // shift characters
            for(size_t i = 1; i < length; i++) {
                word[i - 1] = word[i];
            }

            // save next
            word[length - 1] = next;
        } while(next);
    }

    passgen_markov_free(&markov);
    passgen_random_close(&rand);

    return test_ok;
}

test_result test_markov_leaf_count_random(void) {
    passgen_random rand;
    passgen_random_xorshift_open(&rand, SEED);
    passgen_markov_leaf *leaf = passgen_markov_leaf_new();

    for(size_t count = 0; count < 10000; count++) {
        uint32_t codepoint = passgen_random_unicode(&rand);
        uint32_t before = passgen_markov_leaf_count(leaf, codepoint);
        assert_eq(before, 0);
    }

    passgen_markov_leaf_free(leaf);
    passgen_random_close(&rand);

    return test_ok;
}

test_result test_markov_leaf_insert_random(void) {
    passgen_random rand;
    passgen_random_xorshift_open(&rand, SEED);
    passgen_markov_leaf *leaf = passgen_markov_leaf_new();

    for(size_t count = 0; count < 1000; count++) {
        uint32_t codepoint = passgen_random_unicode(&rand);
        uint32_t weight = passgen_random_u16(&rand);
        uint32_t before = passgen_markov_leaf_count(leaf, codepoint);
        leaf = passgen_markov_leaf_insert(leaf, codepoint, weight);
        uint32_t after = passgen_markov_leaf_count(leaf, codepoint);
        assert_eq(after - before, weight);
    }

    // they are sorted, and add up to the total.
    size_t total = 0;
    for(size_t i = 0; i < leaf->len; i++) {
        if(i) {
            assert(leaf->entries[i - 1].codepoint < leaf->entries[i].codepoint);
        }
        total += leaf->entries[i].count;
    }
    assert_eq(total, leaf->total_count);

    passgen_markov_leaf_free(leaf);
    passgen_random_close(&rand);

    return test_ok;
}

test_result test_markov_leaf_insert_sequential(void) {
    passgen_random rand;
    passgen_random_xorshift_open(&rand, SEED);
    passgen_markov_leaf *leaf = passgen_markov_leaf_new();

    for(size_t count = 0; count < 100000; count++) {
        uint32_t codepoint = count;
        uint32_t weight = passgen_random_u16(&rand);
        uint32_t before = passgen_markov_leaf_count(leaf, codepoint);
        leaf = passgen_markov_leaf_insert(leaf, codepoint, weight);
        uint32_t after = passgen_markov_leaf_count(leaf, codepoint);
        assert_eq(after - before, weight);
    }

    passgen_markov_leaf_free(leaf);
    passgen_random_close(&rand);

    return test_ok;
}

/// What is generated only depends on the words, not on the order in which
/// they were added: the codepoints which can follow are chosen from in the
/// order of their codepoints.
test_result test_markov_generate(void) {
    const uint32_t words[][1] = {{'b'}, {'c'}, {'a'}};
    passgen_markov forward, backward;
    passgen_markov_init(&forward, 2);
    passgen_markov_init(&backward, 2);
    for(size_t i = 0; i < 3; i++) {
        assert_eq(PASSGEN_OK, passgen_markov_add(&forward, words[i], 1, 1));
        assert_eq(PASSGEN_OK, passgen_markov_add(&backward, words[2 - i], 1, 1));
    }

    // with zeroes for randomness, the first choice is always made.
    passgen_random *zero = passgen_random_open(NULL, "zero");
    assert(zero);
    const uint32_t start[2] = {0, 0};
    double entropy = 0;
    assert_eq(passgen_markov_generate(&forward, start, zero, &entropy), 'a');
    assert_eq(passgen_markov_generate(&backward, start, zero, NULL), 'a');
    assert(entropy > 1.58 && entropy < 1.59);

    // only the end of the word can follow, which takes no randomness.
    const uint32_t after[2] = {0, 'a'};
    entropy = 0;
    assert_eq(passgen_markov_generate(&forward, after, zero, &entropy), 0);
    assert(entropy == 0);

    // with the same randomness, they generate the same.
    passgen_random *one = passgen_random_open(NULL, "xorshift:1");
    passgen_random *two = passgen_random_open(NULL, "xorshift:1");
    assert(one && two);
    for(size_t i = 0; i < 100; i++) {
        assert_eq(
            passgen_markov_generate(&forward, start, one, NULL),
            passgen_markov_generate(&backward, start, two, NULL));
    }

    passgen_random_free(zero);
    passgen_random_free(one);
    passgen_random_free(two);
    passgen_markov_free(&forward);
    passgen_markov_free(&backward);

    return test_ok;
}

/// Whether a codepoint is in a list of them.
static bool contains(const uint32_t *list, size_t len, uint32_t codepoint) {
    for(size_t i = 0; i < len; i++) {
        if(list[i] == codepoint) {
            return true;
        }
    }
    return false;
}

/// Finding codepoints works for lists of any shape: dense runs, gaps, the
/// zero which ends words, codepoints far from the others, and the limits.
test_result test_markov_leaf_search_uneven(void) {
    const uint32_t extra[] = {0, 1, '\'', '-', 0xE9, 0x4E00, 0x1F600, 0x10FFFF};
    passgen_random rand;
    passgen_random_xorshift_open(&rand, SEED);

    for(size_t shape = 0; shape < 256; shape++) {
        passgen_markov_leaf *leaf = passgen_markov_leaf_new();
        assert(leaf);

        // some of the letters, and the extra ones which the shape says.
        uint32_t inserted[64];
        size_t count = 0;
        for(uint32_t c = 'a'; c <= 'z'; c++) {
            if(passgen_random_u8(&rand) % 4) {
                inserted[count++] = c;
            }
        }
        for(size_t i = 0; i < 8; i++) {
            if((shape >> i) & 1) {
                inserted[count++] = extra[i];
            }
        }
        // in any order, so that they go into the middle as well.
        for(size_t i = count; i > 1; i--) {
            size_t j = passgen_random_u64_max(&rand, i);
            uint32_t swap = inserted[i - 1];
            inserted[i - 1] = inserted[j];
            inserted[j] = swap;
        }
        for(size_t i = 0; i < count; i++) {
            leaf = passgen_markov_leaf_insert(leaf, inserted[i], 1);
            assert(leaf);
        }
        assert_eq(leaf->len, count);

        // every one which is there is found, and its neighbours are only
        // found if they are there too.
        for(size_t i = 0; i < count; i++) {
            for(int offset = -1; offset <= 1; offset++) {
                uint32_t codepoint = inserted[i] + offset;
                if(inserted[i] == 0 && offset < 0) {
                    continue;
                }
                uint32_t expected = contains(inserted, count, codepoint) ? 1 : 0;
                assert_eq(passgen_markov_leaf_count(leaf, codepoint), expected);
            }
        }
        assert_eq(passgen_markov_leaf_count(leaf, 0x10FFFE), 0);
        assert_eq(passgen_markov_leaf_count(leaf, 'Z'), 0);

        // and they are in order.
        for(size_t i = 1; i < leaf->len; i++) {
            assert(leaf->entries[i - 1].codepoint < leaf->entries[i].codepoint);
        }

        passgen_markov_leaf_free(leaf);
    }

    passgen_random_close(&rand);
    return test_ok;
}
