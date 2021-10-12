#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>

struct passgen_hashmap_entry;
struct passgen_hashmap_context;
struct passgen_hashmap;

/// Hashmap using open addressing.
typedef struct passgen_hashmap {
    /// The current capacity of the data allocation. If it is zero, there is
    /// no allocation for data.
    size_t capacity;

    /// The amount of items currently allocated.
    size_t len;

    /// The context. This determines the hash function that is used, as well
    /// as the comparison function.
    const struct passgen_hashmap_context *context;

    /// The data allocation. This is an array of *capacity* entries.
    struct passgen_hashmap_entry *data;
} passgen_hashmap;

/// Represents a single entry. If the *key* pointer is NULL, it is unoccupied.
typedef struct passgen_hashmap_entry {
    void *key;
    void *value;
} passgen_hashmap_entry;

/// Determines the hash function and comparison functions that are used. The
/// functions are always also passed a pointer to the hashmap itself, this way
/// additional data can be stored past the actual hashmap, and the functions
/// can read it.
typedef struct passgen_hashmap_context {
    // Determine the hash value of the supplied key
    uint64_t (*hash)(const struct passgen_hashmap *map, const void *key);
    // Determine if two keys are equal
    bool (*key_equal)(const struct passgen_hashmap *map, const void *lhs, const void *rhs);
} passgen_hashmap_context;

/// Default context which assumes that the keys are NULL-terminated UTF-8
/// encoded strings.
extern const passgen_hashmap_context passgen_hashmap_context_default;

/// Assumes that keys are unicode (UTF-32), NULL-terminated strings.
extern const passgen_hashmap_context passgen_hashmap_context_unicode;

/// Initialize new hashmap. If context is NULL, it uses the default context
/// which assumes that the keys are strings.
void passgen_hashmap_init(passgen_hashmap *map, const passgen_hashmap_context *context);

/// Free the hashmap. Beware: this will not free any of the items.
void passgen_hashmap_free(passgen_hashmap *map);

/// Insert new item into the hasmap. If there was a previous entry with the
/// same key, it will return it.
passgen_hashmap_entry passgen_hashmap_insert(passgen_hashmap *map, void *key, void *value);

/// Remove an entry from the hashmap. Returns the entry if found, else returns
/// an empty entry.
passgen_hashmap_entry passgen_hashmap_remove(passgen_hashmap *map, void *key);

/// Lookup an entry in the hashmap. Returns NULL if not found. Do not change
/// the key for this entry.
passgen_hashmap_entry *passgen_hashmap_lookup(passgen_hashmap *map, void *key);

/// Convenience function to run the passed function for every key.
void passgen_hashmap_foreach_key(passgen_hashmap *map, void (*func)(void *key));

/// Convenience function to run the passed function for every value.
void passgen_hashmap_foreach_value(passgen_hashmap *map, void (*func)(void *value));
