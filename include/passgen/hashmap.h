#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>

struct passgen_hashmap_entry;
struct passgen_hashmap_context;
struct passgen_hashmap;

typedef struct passgen_hashmap {
    size_t capacity;
    size_t len;
    const struct passgen_hashmap_context *context;
    struct passgen_hashmap_entry *data;
} passgen_hashmap;

typedef struct passgen_hashmap_entry {
    void *key;
    void *data;
} passgen_hashmap_entry;

typedef struct passgen_hashmap_context {
    // Determine the hash value of the supplied key
    uint64_t (*hash)(const struct passgen_hashmap *map, const void *key);
    // Determine if two keys are equal
    bool (*key_equal)(const struct passgen_hashmap *map, const void *lhs, const void *rhs);
} passgen_hashmap_context;

const passgen_hashmap_context passgen_hashmap_context_default;
const passgen_hashmap_context passgen_hashmap_context_unicode;

// Initialize new hashmap. If context is NULL, it uses the default context
// which assumes that the keys are strings.
void passgen_hashmap_init(passgen_hashmap *map, const passgen_hashmap_context *context);

// Free the hashmap. Beware: this will not free any of the items.
void passgen_hashmap_free(passgen_hashmap *map);

// Insert new item into the hasmap. If there was a previous entry with the
// same key, it will return it.
passgen_hashmap_entry passgen_hashmap_insert(passgen_hashmap *map, void *key, void *data);

// Remove an entry from the hashmap. Returns the entry if found, else returns
// an empty entry.
passgen_hashmap_entry passgen_hashmap_remove(passgen_hashmap *map, void *key);

// Lookup an entry in the hashmap. Returns NULL if not found. Do not change
// the key for this entry.
passgen_hashmap_entry *passgen_hashmap_lookup(passgen_hashmap *map, void *key);

