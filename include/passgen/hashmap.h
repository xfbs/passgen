/// @file hashmap.h
/// @author Patrick M. Elsen <pelsen@xfbs.net>
/// @brief HashMap implementation
///
/// This implements a hashmap with a variable hash function and open addressing.

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>

struct passgen_hashmap_entry;
struct passgen_hashmap_context;
struct passgen_hashmap;

/// Hashmap implementation with variable hash function and open addressing.
///
/// This hashmap implementation tries to be space efficient and yet simple.
/// To achieve that, it uses two-way hashing: for every value, there are two
/// possible hashes (this could be generalised to n hashes). For every
/// operation (insertion, lookup, deletion), it tries to use the first hash
/// and falls back to using the second. This increases the hashmap space
/// efficiency 10x in tests. Finally, when inserting, if both slots are
/// taken, it attempts to see if it can move around either of the two items.
/// This leads to another 2x space efficiency increase at the expense of minor
/// complexity when inserting.
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
    uint64_t (*hash)(const struct passgen_hashmap *map, const void *key, bool first);
    // Determine if two keys are equal
    bool (*key_equal)(const struct passgen_hashmap *map, const void *lhs, const void *rhs);
} passgen_hashmap_context;

/// Default context which assumes that the keys are NULL-terminated UTF-8
/// encoded strings.
extern const passgen_hashmap_context passgen_hashmap_context_default;

/// Assumes that keys are unicode (UTF-32), NULL-terminated strings.
extern const passgen_hashmap_context passgen_hashmap_context_unicode;

/// Initialize new hashmap. If context is NULL, it uses the default context
/// which assumes that the keys are NULL-terminated, UTF-8 strings.
void passgen_hashmap_init(passgen_hashmap *map, const passgen_hashmap_context *context);

/// Free the hashmap.
///
/// Beware: this will not free any of the items. Use `passgen_hashmap_foreach_key`
/// or `passgen_hashmap_foreach_value` to free the items.
void passgen_hashmap_free(passgen_hashmap *map);

/// Insert new item into the hashmap. If there was a previous entry with the
/// same key, it will return it so that it can be deallocated. Will overwrite
/// both key and value.
passgen_hashmap_entry passgen_hashmap_insert(passgen_hashmap *map, void *key, void *value);

/// Remove an entry from the hashmap. Returns the entry if found, else returns
/// an empty entry (where `key` == NULL).
passgen_hashmap_entry passgen_hashmap_remove(passgen_hashmap *map, void *key);

/// Lookup an entry in the hashmap. Returns NULL if not found.
///
/// Feel free to change the value via this pointer. However, the returned entry
/// is only valid until the next change to the hashmap (insertion, deletion).
/// Do not change the key via this pointer.
passgen_hashmap_entry *passgen_hashmap_lookup(passgen_hashmap *map, void *key);

/// Run the passed function for every key.
void passgen_hashmap_foreach_key(passgen_hashmap *map, void (*func)(void *key));

/// Run the passed function for every value.
void passgen_hashmap_foreach_value(passgen_hashmap *map, void (*func)(void *value));
