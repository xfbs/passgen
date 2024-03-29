#include "passgen/container/hashmap.h"
#include "tests.h"
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
    entry = passgen_hashmap_insert(&map, &key1, &value1);
    assert_eq(entry.key, NULL);
    assert_eq(entry.value, NULL);
    assert_eq(map.len, 1);
    assert_eq(passgen_hashmap_lookup(&map, &key1)->value, &value1);

    uint32_t key2[] = {'h', 'o', 'l', 'a', 0};
    int value2;
    entry = passgen_hashmap_insert(&map, &key2, &value2);
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

    passgen_hashmap_insert(&map, "hello", &value_a);
    assert_eq(map.len, 1);
    passgen_hashmap_insert(&map, "world", &value_b);
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
        passgen_hashmap_insert(&map, key, value);

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

    passgen_hashmap_insert(&map, "hello", &value_a);
    passgen_hashmap_insert(&map, "world", &value_b);
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

    passgen_hashmap_insert(&map, "alpha", malloc(255));
    passgen_hashmap_insert(&map, "beta", malloc(255));
    passgen_hashmap_insert(&map, "gamma", malloc(255));
    passgen_hashmap_insert(&map, "delta", malloc(255));
    passgen_hashmap_insert(&map, "epsilon", malloc(255));

    assert_eq(passgen_hashmap_foreach(&map, NULL, function_fails), 7);
    assert_eq(passgen_hashmap_foreach(&map, NULL, free_value), 0);

    passgen_hashmap_free(&map);
    return test_ok;
}
