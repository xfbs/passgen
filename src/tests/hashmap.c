#include "tests.h"
#include "passgen/hashmap.h"

test_result test_hashmap_init(void) {
    passgen_hashmap map;
    passgen_hashmap_init(&map, NULL);
    passgen_hashmap_free(&map);
    return test_ok;
}

test_result test_hashmap_insert(void) {
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
    assert_eq(entry->data, &value_a);

    entry = passgen_hashmap_lookup(&map, "world");
    assert(entry);
    assert_eq(entry->data, &value_b);

    entry = passgen_hashmap_lookup(&map, "other");
    assert(!entry);

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
    assert_eq(entry.data, &value_a);
    assert_streq(entry.key, "hello");

    assert(!passgen_hashmap_lookup(&map, "hello"));
    assert(passgen_hashmap_lookup(&map, "world"));
    assert(!passgen_hashmap_lookup(&map, "other"));

    entry = passgen_hashmap_remove(&map, "world");
    assert_eq(map.len, 0);
    assert(entry.key);
    assert_eq(entry.data, &value_b);
    assert_streq(entry.key, "world");

    assert(!passgen_hashmap_lookup(&map, "hello"));
    assert(!passgen_hashmap_lookup(&map, "world"));
    assert(!passgen_hashmap_lookup(&map, "other"));

    passgen_hashmap_free(&map);
    return test_ok;
}
