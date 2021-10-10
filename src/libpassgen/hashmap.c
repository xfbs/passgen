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

void passgen_hashmap_realloc(passgen_hashmap *map, size_t capacity) {
    // save previous hashmap
    passgen_hashmap old = *map;

    map->data = calloc(capacity, sizeof(passgen_hashmap_entry));
    map->capacity = capacity;

    // copy data over
    for(size_t i = 0; i < old.capacity; i++) {
        if(old.data[i].key) {
            passgen_hashmap_insert(map, old.data[i].key, old.data[i].data);
        }
    }

    free(old.data);
}

static inline size_t passgen_hashmap_position(passgen_hashmap *map, void *key) {
    return map->context->hash(map, key) % map->capacity;
}

passgen_hashmap_entry passgen_hashmap_insert(passgen_hashmap *map, void *key, void *data) {
    if(map->capacity == 0) {
        passgen_hashmap_realloc(map, hashmap_sizes[0]);
    }

    size_t hash = passgen_hashmap_position(map, key);
    if(!map->data[hash].key) {
        map->data[hash].key = key;
        map->data[hash].data = data;
        map->len += 1;
        return (passgen_hashmap_entry){NULL, NULL};
    } else {
        if(map->context->key_equal(map, map->data[hash].key, key)) {
            passgen_hashmap_entry entry = map->data[hash];
            map->data[hash].key = key;
            map->data[hash].data = data;
            return entry;
        } else {
            passgen_hashmap_realloc(map, 2 * map->capacity);
            return passgen_hashmap_insert(map, key, data);
        }
    }
}

passgen_hashmap_entry passgen_hashmap_remove(passgen_hashmap *map, void *key) {
    size_t hash = passgen_hashmap_position(map, key);

    // if this key exists
    if(map->data[hash].key) {
        // and it matches
        if(map->context->key_equal(map, map->data[hash].key, key)) {
            passgen_hashmap_entry entry = map->data[hash];
            map->data[hash].key = NULL;
            map->data[hash].data = NULL;
            map->len -= 1;
            return entry;
        }
    }

    return (passgen_hashmap_entry){NULL, NULL};
}

passgen_hashmap_entry *passgen_hashmap_lookup(passgen_hashmap *map, void *key) {
    size_t hash = passgen_hashmap_position(map, key);
    if(map->data[hash].key && map->context->key_equal(map, map->data[hash].key, key)) {
        return &map->data[hash];
    } else {
        return NULL;
    }
}

uint64_t string_hash(const passgen_hashmap *map, const void *key) {
    uint64_t output;
    passgen_siphash(key, strlen(key), "key", &output, sizeof(output));
    return output;
}

bool string_equal(const passgen_hashmap *map, const void *lhs, const void *rhs) {
    return strcmp(lhs, rhs) == 0;
}

const passgen_hashmap_context passgen_hashmap_context_default = {
    .hash = string_hash,
    .key_equal = string_equal,
};

size_t unicode_len(const void *data) {
    const int32_t *unicode = data;
    size_t len = 0;
    while(unicode[len]) {
        len++;
    }
    return len;
}

uint64_t unicode_hash(const passgen_hashmap *map, const void *key) {
    uint64_t output;
    passgen_siphash(key, unicode_len(key), "key", &output, sizeof(output));
    return output;
}

bool unicode_equal(const passgen_hashmap *map, const void *lhs, const void *rhs) {
    size_t rhs_len = unicode_len(rhs);
    size_t lhs_len = unicode_len(lhs);
    if(rhs_len != lhs_len) {
        return false;
    }
    return memcmp(lhs, rhs, rhs_len) == 0;
}

const passgen_hashmap_context passgen_hashmap_context_unicode = {
    .hash = unicode_hash,
    .key_equal = unicode_equal,
};
