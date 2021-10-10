#include "passgen/hashmap.h"
#include "passgen/siphash.h"
#include <stdlib.h>

static const hashmap_sizes[] = {3, 7, 17, 37, 79, 163, 331, 673, 1361, 2729, 5471, 10949, 21911, 43853, 87719, 175447, 350899, 701819};

void passgen_hashmap_init(passgen_hashmap *map, const passgen_hashmap_context *context) {
    memset(map, 0, sizeof(*map));
    if(context) {
        map->context = context;
    } else {
        map->context = &passgen_hashmap_context_default;
    }
}

void passgen_hashmap_free(passgen_hashmap *map) {
    free(map->data);
    memset(map, 0, sizeof(*map));
}

void passgen_hashmap_realloc(passgen_hashmap *map, size_t size) {
    // save previous hashmap
    passgen_hashmap old = *map;

    map->data = calloc(size, sizeof(passgen_hashmap_entry));
    map->capacity = size;

    // copy data over
    for(size_t i = 0; i < old.capacity; i++) {
        if(old.data[i].key) {
        }
    }

    free(old.data);
}

passgen_hashmap_entry passgen_hashmap_insert(passgen_hashmap *map, void *key, void *data) {
    if(map->capacity == 0) {
        passgen_hashmap_realloc(map, hashmap_sizes[0]);
    }
    uint64_t hash;

    hash = map->context->hash_one(map, key) % map->capacity;
    if(!map->data[hash].key) {
        map->data[hash].key = key;
        map->data[hash].data = data;
        return (passgen_hashmap_entry){NULL, NULL};
    } else {
        if(map->context->key_equal(map, map->data[hash].key, key)) {
            passgen_hashmap_entry entry = map->data[hash];
            map->data[hash].key = key;
            map->data[hash].data = data;
            return entry;
        } else {
        }
    }
}

passgen_hashmap_entry passgen_hashmap_remove(passgen_hashmap *map, void *key) {
}

passgen_hashmap_entry *passgen_hashmap_lookup(passgen_hashmap *map, void *key) {
    uint64_t hash = map->context->hash_one(map, key) % map->capacity;
    if(map->data[hash].key && map->context->key_equal(map, map->data[hash].key, key)) {
        return &map->data[hash];
    } else {
        return NULL;
    }
}

uint64_t hash_string(const passgen_hashmap *map, const void *key) {
    uint64_t output;
    passgen_siphash(key, strlen(key), "key", &output, sizeof(output));
    return output;
}

uint64_t hash_string_two(const passgen_hashmap *map, const void *key) {
    uint64_t output;
    passgen_siphash(key, strlen(key), "other", &output, sizeof(output));
    return output;
}

bool equal_string(const passgen_hashmap *map, const void *lhs, const void *rhs) {
    return strcmp(lhs, rhs) == 0;
}

const passgen_hashmap_context passgen_hashmap_context_default = {
    .hash_one = hash_string,
    .hash_two = hash_string_two,
    .key_equal = equal_string,
};
