#include "passgen/container/hashmap.h"
#include "testalloc.h"
#include "tests.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

test_result test_hashmap_init(void) {
    passgen_hashmap map;
    passgen_hashmap_init(&map, NULL);
    passgen_hashmap_free(&map);
    return test_ok;
}

test_result test_hashmap_context_utf8(void) {
    passgen_hashmap map;
    passgen_hashmap_init(&map, &passgen_hashmap_context_utf8);

    const char *key1 = "hello";
    const char *key2 = "hola";

    assert(passgen_hashmap_context_utf8.equal(&map, key1, key1));
    assert(passgen_hashmap_context_utf8.equal(&map, key2, key2));
    assert(!passgen_hashmap_context_utf8.equal(&map, key1, key2));

    assert_eq(
        passgen_hashmap_context_utf8.hash(&map, key1, true),
        0xf6f0cd43722c7cfe);
    assert_eq(
        passgen_hashmap_context_utf8.hash(&map, key1, false),
        0x438bcb516e4d4abf);

    assert_eq(
        passgen_hashmap_context_utf8.hash(&map, key2, true),
        0xcb99aa476d12f348);
    assert_eq(
        passgen_hashmap_context_utf8.hash(&map, key2, false),
        0xef7ec81d7aa533ea);

    passgen_hashmap_free(&map);

    return test_ok;
}

test_result test_hashmap_context_utf32(void) {
    passgen_hashmap map;
    passgen_hashmap_init(&map, &passgen_hashmap_context_utf32);

    uint32_t key1[] = {'h', 'e', 'l', 'l', 'o', 0};
    uint32_t key2[] = {'h', 'o', 'l', 'a', 0};

    assert(passgen_hashmap_context_utf32.equal(&map, &key1[0], &key1[0]));
    assert(passgen_hashmap_context_utf32.equal(&map, &key2[0], &key2[0]));
    assert(!passgen_hashmap_context_utf32.equal(&map, &key1[0], &key2[0]));

    assert_eq(
        passgen_hashmap_context_utf32.hash(&map, &key1[0], true),
        0x4b9a41e6925a46ad);
    assert_eq(
        passgen_hashmap_context_utf32.hash(&map, &key1[0], false),
        0x6ce43ac01a6144b8);

    assert_eq(
        passgen_hashmap_context_utf32.hash(&map, &key2[0], true),
        0xdefab330a5dcde19);
    assert_eq(
        passgen_hashmap_context_utf32.hash(&map, &key2[0], false),
        0x45168f9b31b7e7bd);

    passgen_hashmap_free(&map);

    return test_ok;
}

test_result test_hashmap_insert_utf32(void) {
    passgen_hashmap map;
    passgen_hashmap_entry entry;

    passgen_hashmap_init(&map, &passgen_hashmap_context_utf32);

    uint32_t key1[] = {'h', 'e', 'l', 'l', 'o', 0};
    int value1;
    assert(PASSGEN_OK == passgen_hashmap_insert(&map, &key1, &value1, &entry));
    assert_eq(entry.key, NULL);
    assert_eq(entry.value, NULL);
    assert_eq(map.len, 1);
    assert_eq(passgen_hashmap_lookup(&map, &key1)->value, &value1);

    uint32_t key2[] = {'h', 'o', 'l', 'a', 0};
    int value2;
    assert(PASSGEN_OK == passgen_hashmap_insert(&map, &key2, &value2, &entry));
    assert_eq(map.len, 2);
    assert_eq(entry.key, NULL);
    assert_eq(entry.value, NULL);
    assert_eq(passgen_hashmap_lookup(&map, &key1)->value, &value1);
    assert_eq(passgen_hashmap_lookup(&map, &key2)->value, &value2);

    passgen_hashmap_free(&map);

    return test_ok;
}

test_result test_hashmap_insert_utf8(void) {
    passgen_hashmap map;
    passgen_hashmap_init(&map, NULL);

    int value_a = 4, value_b = 7;

    assert(PASSGEN_OK == passgen_hashmap_insert(&map, "hello", &value_a, NULL));
    assert_eq(map.len, 1);
    assert(PASSGEN_OK == passgen_hashmap_insert(&map, "world", &value_b, NULL));
    assert_eq(map.len, 2);

    passgen_hashmap_entry *entry;
    entry = passgen_hashmap_lookup(&map, "hello");
    assert(entry);
    assert_eq(entry->value, &value_a);

    entry = passgen_hashmap_lookup(&map, "world");
    assert(entry);
    assert_eq(entry->value, &value_b);

    entry = passgen_hashmap_lookup(&map, "other");
    assert(!entry);

    passgen_hashmap_free(&map);
    return test_ok;
}

test_result test_hashmap_insert_many(void) {
    passgen_hashmap map;
    passgen_hashmap_init(&map, NULL);
    size_t insert_max = 10000;

    // insert large amount of items
    for(size_t i = 0; i < insert_max; i++) {
        assert_eq(map.len, i);
        char *key = malloc(32);
        size_t *value = malloc(sizeof(size_t));
        *value = i;
        snprintf(key, 32, "%zu", i);
        assert(PASSGEN_OK == passgen_hashmap_insert(&map, key, value, NULL));

        // fill factor is how many percent of slots are used. we want this
        // to be high.
        float fill = map.len * 100.0 / (double) map.capacity;
        assert(fill > 3.0);
    }

    // retrieve all items
    for(size_t i = 0; i < insert_max; i++) {
        char key[32];
        snprintf(key, 32, "%zu", i);
        passgen_hashmap_entry *entry = passgen_hashmap_lookup(&map, key);
        assert(entry);
        size_t *value = entry->value;
        assert_eq(*value, i);
    }

    // release memory
    passgen_hashmap_foreach(&map, NULL, passgen_hashmap_entry_free);

    passgen_hashmap_free(&map);
    return test_ok;
}

test_result test_hashmap_remove(void) {
    passgen_hashmap map;
    passgen_hashmap_init(&map, NULL);

    int value_a = 4, value_b = 7;

    assert(PASSGEN_OK == passgen_hashmap_insert(&map, "hello", &value_a, NULL));
    assert(PASSGEN_OK == passgen_hashmap_insert(&map, "world", &value_b, NULL));
    assert_eq(map.len, 2);

    assert(passgen_hashmap_lookup(&map, "hello"));
    assert(passgen_hashmap_lookup(&map, "world"));
    assert(!passgen_hashmap_lookup(&map, "other"));

    passgen_hashmap_entry entry;
    entry = passgen_hashmap_remove(&map, "hello");
    assert_eq(map.len, 1);
    assert(entry.key);
    assert_eq(entry.value, &value_a);
    assert_streq(entry.key, "hello");

    assert(!passgen_hashmap_lookup(&map, "hello"));
    assert(passgen_hashmap_lookup(&map, "world"));
    assert(!passgen_hashmap_lookup(&map, "other"));

    entry = passgen_hashmap_remove(&map, "world");
    assert_eq(map.len, 0);
    assert(entry.key);
    assert_eq(entry.value, &value_b);
    assert_streq(entry.key, "world");

    assert(!passgen_hashmap_lookup(&map, "hello"));
    assert(!passgen_hashmap_lookup(&map, "world"));
    assert(!passgen_hashmap_lookup(&map, "other"));

    passgen_hashmap_free(&map);
    return test_ok;
}

test_result test_hashmap_lookup_empty(void) {
    passgen_hashmap map;
    passgen_hashmap_init(&map, NULL);

    passgen_hashmap_entry *entry = passgen_hashmap_lookup(&map, "test");
    assert(!entry);

    passgen_hashmap_free(&map);
    return test_ok;
}

int function_fails(void *data, passgen_hashmap_entry *entry) {
    (void) data;
    (void) entry;
    return 7;
}

int free_value(void *data, passgen_hashmap_entry *entry) {
    (void) data;
    free(entry->value);
    return 0;
}

test_result test_hashmap_foreach(void) {
    passgen_hashmap map;
    passgen_hashmap_init(&map, NULL);

    assert(PASSGEN_OK == passgen_hashmap_insert(&map, "alpha", malloc(255), NULL));
    assert(PASSGEN_OK == passgen_hashmap_insert(&map, "beta", malloc(255), NULL));
    assert(PASSGEN_OK == passgen_hashmap_insert(&map, "gamma", malloc(255), NULL));
    assert(PASSGEN_OK == passgen_hashmap_insert(&map, "delta", malloc(255), NULL));
    assert(PASSGEN_OK == passgen_hashmap_insert(&map, "epsilon", malloc(255), NULL));

    assert_eq(passgen_hashmap_foreach(&map, NULL, function_fails), 7);
    assert_eq(passgen_hashmap_foreach(&map, NULL, free_value), 0);

    passgen_hashmap_free(&map);
    return test_ok;
}

// a context where the positions of keys are chosen by the test. keys are
// pointers to a pair of positions.
typedef struct {
    size_t first;
    size_t second;
} chosen_key;

static uint64_t
chosen_hash(const passgen_hashmap *map, const void *key, bool first) {
    (void) map;
    const chosen_key *chosen = key;
    return first ? chosen->first : chosen->second;
}

static bool
chosen_equal(const passgen_hashmap *map, const void *lhs, const void *rhs) {
    (void) map;
    return lhs == rhs;
}

static void chosen_init(passgen_hashmap *map) {
    (void) map;
}

static const passgen_hashmap_context chosen_context = {
    .hash = chosen_hash,
    .equal = chosen_equal,
    .init = chosen_init,
    .fini = chosen_init,
};

/// Inserting a key which is in its second position, while its first is
/// taken by another key that could be moved, used to move that one and put
/// the key in the hashmap a second time.
test_result test_hashmap_insert_existing_second(void) {
    passgen_hashmap map;
    passgen_hashmap_init(&map, &chosen_context);

    // the hashmap starts with three positions.
    chosen_key both_zero = {0, 0};
    chosen_key key = {0, 1};
    chosen_key other = {0, 2};
    int value;

    // the key goes into its second position, as both_zero cannot move.
    assert(PASSGEN_OK == passgen_hashmap_insert(&map, &both_zero, NULL, NULL));
    assert(PASSGEN_OK == passgen_hashmap_insert(&map, &key, NULL, NULL));
    assert_eq(map.capacity, 3);
    assert(map.data[1].key == &key);
    passgen_hashmap_remove(&map, &both_zero);

    // other takes the first position of key, and could move to its second.
    assert(PASSGEN_OK == passgen_hashmap_insert(&map, &other, NULL, NULL));
    assert(map.data[0].key == &other);
    assert_eq(map.len, 2);

    passgen_hashmap_entry previous;
    assert(PASSGEN_OK == passgen_hashmap_insert(&map, &key, &value, &previous));
    assert(previous.key == &key);
    assert_eq(map.len, 2);
    size_t found = 0;
    for(size_t i = 0; i < map.capacity; i++) {
        if(map.data[i].key == &key) {
            found++;
            assert(map.data[i].value == &value);
        }
    }
    assert_eq(found, 1);

    passgen_hashmap_free(&map);

    return test_ok;
}

/// Running out of memory at any point while inserting leaves the hashmap as
/// it was, with all of what was in it.
test_result test_hashmap_out_of_memory(void) {
    const size_t count = 200;
    char keys[200][8];
    for(size_t i = 0; i < count; i++) {
        snprintf(keys[i], sizeof(keys[i]), "%zu", i);
    }

    bool done = false;
    for(size_t fail_at = 0; !done; fail_at++) {
        test_allocator allocator;
        test_allocator_install(&allocator, fail_at);

        passgen_hashmap map;
        passgen_hashmap_init(&map, NULL);

        size_t inserted = 0;
        while(inserted < count) {
            passgen_status status = passgen_hashmap_insert(
                &map,
                keys[inserted],
                &keys[inserted],
                NULL);
            if(status != PASSGEN_OK) {
                assert(status == PASSGEN_ERROR_OUT_OF_MEMORY);
                break;
            }
            inserted++;
        }

        // what failed once works once there is memory again.
        if(inserted < count) {
            assert_eq(map.len, inserted);
            allocator.fail_at = TEST_ALLOCATOR_NEVER;
            assert(
                PASSGEN_OK == passgen_hashmap_insert(
                                  &map,
                                  keys[inserted],
                                  &keys[inserted],
                                  NULL));
            inserted++;
        } else {
            done = true;
        }

        assert_eq(map.len, inserted);
        for(size_t i = 0; i < inserted; i++) {
            passgen_hashmap_entry *entry = passgen_hashmap_lookup(&map, keys[i]);
            assert(entry);
            assert(entry->value == &keys[i]);
        }

        passgen_hashmap_free(&map);
        assert(test_allocator_clean(&allocator));
        test_allocator_remove();
    }

    return test_ok;
}
