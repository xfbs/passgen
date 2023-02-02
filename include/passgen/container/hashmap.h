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
///
/// @relates passgen_hashmap
typedef struct passgen_hashmap_entry {
    /// Key of the entry.
    ///
    /// This must not be changed. If you want to change this key, you must
    /// first remove it from the hashmap using #passgen_hashmap_remove and then
    /// re-add it with #passgen_hashmap_insert.
    const void *key;
    /// Value of this entry.
    void *value;
} passgen_hashmap_entry;

/// Context, this determines the hash function and comparison functions that are used.
///
/// When these functions are called, they are always also passed a pointer to
/// the hashmap itself. This way, additional data can be stored in the memory
/// past the hashmap that the functions can use. This can be used to store
/// additional state.
///
/// @relates passgen_hashmap
typedef struct passgen_hashmap_context {
    // Determine the hash value of the supplied key
    uint64_t (*hash)(const struct passgen_hashmap *map, const void *key, bool first);
    // Determine if two keys are equal
    bool (*equal)(const struct passgen_hashmap *map, const void *lhs, const void *rhs);
} passgen_hashmap_context;

/// Default context which assumes that the keys are NULL-terminated UTF-8
/// encoded strings.
///
/// @relates passgen_hashmap
extern const passgen_hashmap_context passgen_hashmap_context_utf8;

/// Assumes that keys are unicode (UTF-32), NULL-terminated strings.
///
/// @relates passgen_hashmap
extern const passgen_hashmap_context passgen_hashmap_context_unicode;

/// Initialize new hashmap. If context is NULL, it uses the default context
/// which assumes that the keys are NULL-terminated, UTF-8 strings.
///
/// @memberof passgen_hashmap
/// @param map Hashmap to initialize
/// @param context Context to to set for the hashmap
void passgen_hashmap_init(passgen_hashmap *map, const passgen_hashmap_context *context);

/// Free the hashmap.
///
/// @memberof passgen_hashmap
/// @param map HashMap to free
///
/// Beware: this will not free any of the items. Use `passgen_hashmap_foreach_key`
/// or `passgen_hashmap_foreach_value` to free the items.
void passgen_hashmap_free(passgen_hashmap *map);

/// Insert new item into the hashmap. If there was a previous entry with the
/// same key, it will return it so that it can be deallocated. Will overwrite
/// both key and value.
///
/// @memberof passgen_hashmap
/// @param map Hashamp to insert into
/// @param key Key to insert at
/// @param valye Value to insert
/// @returns Previous entry for the same key if one existed, else NULL.
passgen_hashmap_entry passgen_hashmap_insert(passgen_hashmap *map, const void *key, void *value);

/// Remove an entry from the hashmap. Returns the entry if found, else returns
/// an empty entry (where `key` == NULL).
///
/// @memberof passgen_hashmap
/// @param map Hashamp to remove from into
/// @param key Key to remove
/// @returns Entry for the key if one existed, else NULL.
passgen_hashmap_entry passgen_hashmap_remove(passgen_hashmap *map, const void *key);

/// Lookup an entry in the hashmap. Returns NULL if not found.
///
/// Feel free to change the value via this pointer. However, the returned entry
/// is only valid until the next change to the hashmap (insertion, deletion).
/// Do not change the key via this pointer.
///
/// @memberof passgen_hashmap
/// @param map Hashamp to lookup in
/// @param key Key to lookup
/// @returns Entry for the key if one exists, else NULL.
passgen_hashmap_entry *passgen_hashmap_lookup(const passgen_hashmap *map, const void *key);

/// Run the passed function for every value.
///
/// @memberof passgen_hashmap
/// @param map Hashamp loop over
/// @param user Arbitrary user data that is passed to the function
/// @param func Function that is called for every #passgen_hashmap_entry in the
///   hashmap. If the function returns non-zero, the iteration is aborted and the
///   return value of the function is returned.
/// @returns Zero on success, whatever the #func returned on failure.
int passgen_hashmap_foreach(const passgen_hashmap *map, void *user, int (*func)(void *user, passgen_hashmap_entry *entry));

/// Free both key and entry in hashmap.
///
/// @memberof passgen_hashmap
/// @param user Ignored
/// @param entry Hashmap entry, both `key` and `value` are `free()`d.
/// @returns Zero.
int passgen_hashmap_entry_free(void *user, passgen_hashmap_entry *entry);
