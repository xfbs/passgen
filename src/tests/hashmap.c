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

    passgen_hashmap_insert(&map, "hello", NULL);
    passgen_hashmap_insert(&map, "world", NULL);

    passgen_hashmap_entry *entry;
    entry = passgen_hashmap_lookup(&map, "hello");
    assert(entry);

    entry = passgen_hashmap_lookup(&map, "world");
    assert(entry);

    entry = passgen_hashmap_lookup(&map, "other");
    assert(!entry);

    passgen_hashmap_free(&map);
    return test_ok;
}
