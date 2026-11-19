#include "passgen/container/hashmap.h"
#include "passgen/config.h"
#include "passgen/util/alloc.h"
#include "passgen/util/clear.h"
#include "passgen/util/siphash.h"
#include <stdlib.h>
#include <string.h>
#define UNUSED(x) (void) x

static const size_t hashmap_sizes[] = {
    3,
    7,
    17,
    37,
    79,
    163,
    331,
    673,
    1361,
    2729,
    5471,
    10949,
    21911,
    43853,
    87719,
    175447,
    350899,
    701819,
    0};

struct siphash_keys {
    const char *first;
    const char *second;
};

static const struct siphash_keys utf8_keys = {
    .first = "someverylongpass",
    .second = "myotherverylongk",
};

static const struct siphash_keys utf32_keys = {
    .first = "someverylongpass",
    .second = "myotherverylongk",
};

void passgen_hashmap_init(
    passgen_hashmap *map,
    const passgen_hashmap_context *context) {
    // zero-out hashmap
    memset(map, 0, sizeof(*map));

    // set context or default to UTF-8 context
    if(context) {
        map->context = context;
    } else {
        map->context = &passgen_hashmap_context_utf8;
    }

    // initialize hashmap context
    map->context->init(map);
}

void passgen_hashmap_free(passgen_hashmap *map) {
    // deinitialize context
    map->context->fini(map);

    // deallocate data
    passgen_free(map->data, map->capacity * sizeof(passgen_hashmap_entry));

    // set everything to zero
    PASSGEN_CLEAR(map);
}

static inline size_t passgen_hashmap_position(
    const passgen_hashmap *map,
    const void *key,
    bool first) {
    return map->context->hash(map, key, first) % map->capacity;
}

// Moves the entry at the position to the other position for its key, if
// that is free.
static inline bool passgen_hashmap_move(passgen_hashmap *map, size_t pos) {
    const void *key = map->data[pos].key;
    bool first = passgen_hashmap_position(map, key, true) == pos;
    size_t other_pos = passgen_hashmap_position(map, key, !first);
    if(!map->data[other_pos].key) {
        map->data[other_pos] = map->data[pos];
        return true;
    }
    return false;
}

// Puts a key which is not in the hashmap into it, if there is room for it.
static bool
passgen_hashmap_place(passgen_hashmap *map, const void *key, void *value) {
    for(int i = 0; i < 2; i++) {
        size_t pos = passgen_hashmap_position(map, key, i == 0);
        if(!map->data[pos].key || passgen_hashmap_move(map, pos)) {
            map->data[pos].key = key;
            map->data[pos].value = value;
            map->len += 1;
            return true;
        }
    }

    return false;
}

static inline size_t next_capacity(size_t capacity) {
    for(size_t i = 0; hashmap_sizes[i]; i++) {
        if(hashmap_sizes[i] > capacity) {
            return hashmap_sizes[i];
        }
    }
    return 2 * capacity - 1;
}

// Moves all entries into a larger allocation. If they do not all fit, a
// larger one is tried, until they do.
static passgen_status
passgen_hashmap_grow(passgen_hashmap *map, size_t capacity) {
    passgen_hashmap old = *map;

    while(true) {
        passgen_hashmap_entry *data =
            passgen_alloc_zeroed(capacity * sizeof(passgen_hashmap_entry));
        if(!data) {
            *map = old;
            return PASSGEN_ERROR_OUT_OF_MEMORY;
        }

        map->data = data;
        map->capacity = capacity;
        map->len = 0;

        bool placed = true;
        for(size_t i = 0; placed && i < old.capacity; i++) {
            if(old.data[i].key) {
                placed = passgen_hashmap_place(
                    map,
                    old.data[i].key,
                    old.data[i].value);
            }
        }

        if(placed) {
            break;
        }

        passgen_free(data, capacity * sizeof(passgen_hashmap_entry));
        capacity = next_capacity(capacity);
    }

    passgen_free(old.data, old.capacity * sizeof(passgen_hashmap_entry));
    return PASSGEN_OK;
}

passgen_status passgen_hashmap_insert(
    passgen_hashmap *map,
    const void *key,
    void *value,
    passgen_hashmap_entry *previous) {
    if(previous) {
        *previous = (passgen_hashmap_entry){NULL, NULL};
    }

    // if the key is there already, it is replaced. this has to be checked in
    // both places before anything is moved, or it would end up in the
    // hashmap twice.
    passgen_hashmap_entry *entry = passgen_hashmap_lookup(map, key);
    if(entry) {
        if(previous) {
            *previous = *entry;
        }
        entry->key = key;
        entry->value = value;
        return PASSGEN_OK;
    }

    if(map->capacity == 0) {
        if(PASSGEN_OK != passgen_hashmap_grow(map, hashmap_sizes[0])) {
            return PASSGEN_ERROR_OUT_OF_MEMORY;
        }
    }

    while(!passgen_hashmap_place(map, key, value)) {
        if(PASSGEN_OK !=
           passgen_hashmap_grow(map, next_capacity(map->capacity))) {
            return PASSGEN_ERROR_OUT_OF_MEMORY;
        }
    }

    return PASSGEN_OK;
}

passgen_hashmap_entry
passgen_hashmap_remove(passgen_hashmap *map, const void *key) {
    size_t hash;

    // if this key exists
    hash = passgen_hashmap_position(map, key, true);
    if(map->data[hash].key) {
        // and it matches
        if(map->context->equal(map, map->data[hash].key, key)) {
            passgen_hashmap_entry entry = map->data[hash];
            map->data[hash].key = NULL;
            map->data[hash].value = NULL;
            map->len -= 1;
            return entry;
        }
    }

    // if this key exists
    hash = passgen_hashmap_position(map, key, false);
    if(map->data[hash].key) {
        // and it matches
        if(map->context->equal(map, map->data[hash].key, key)) {
            passgen_hashmap_entry entry = map->data[hash];
            map->data[hash].key = NULL;
            map->data[hash].value = NULL;
            map->len -= 1;
            return entry;
        }
    }

    return (passgen_hashmap_entry){NULL, NULL};
}

passgen_hashmap_entry *
passgen_hashmap_lookup(const passgen_hashmap *map, const void *key) {
    // make sure the hashmap is not empty
    if(!map->data) {
        return NULL;
    }

    size_t hash;

    // lookup first possible location
    hash = passgen_hashmap_position(map, key, true);
    if(map->data[hash].key &&
       map->context->equal(map, map->data[hash].key, key)) {
        return &map->data[hash];
    }

    // lookup second possible location
    hash = passgen_hashmap_position(map, key, false);
    if(map->data[hash].key &&
       map->context->equal(map, map->data[hash].key, key)) {
        return &map->data[hash];
    }

    return NULL;
}

int passgen_hashmap_foreach(
    const passgen_hashmap *map,
    void *user,
    int (*func)(void *user, passgen_hashmap_entry *entry)) {
    if(map->len) {
        for(size_t i = 0; i < map->capacity; i++) {
            if(map->data[i].key) {
                int ret = func(user, &map->data[i]);
                if(ret != 0) {
                    return ret;
                }
            }
        }
    }

    return 0;
}

static void utf8_init(passgen_hashmap *map) {
    map->context_data = (void *) &utf8_keys;
}

static void utf8_fini(passgen_hashmap *map) {
    map->context_data = NULL;
}

static uint64_t
utf8_hash(const passgen_hashmap *map, const void *key, bool first) {
    UNUSED(map);
    uint64_t output;
    const struct siphash_keys *keys = map->context_data;
    const char *siphash_key = first ? keys->first : keys->second;
    passgen_siphash(
        key,
        strlen(key),
        siphash_key,
        (uint8_t *) &output,
        sizeof(output));
    return output;
}

static bool
utf8_equal(const passgen_hashmap *map, const void *lhs, const void *rhs) {
    UNUSED(map);
    return strcmp(lhs, rhs) == 0;
}

const passgen_hashmap_context passgen_hashmap_context_utf8 = {
    .hash = utf8_hash,
    .equal = utf8_equal,
    .init = utf8_init,
    .fini = utf8_fini,
};

static size_t utf32_len(const void *data) {
    // cast void pointer to array of UTF-32 codepoints
    const int32_t *utf32 = data;

    // iterate over until we hit zero codepoint
    size_t len = 0;
    while(utf32[len]) {
        len++;
    }

    return len;
}

static uint64_t
utf32_hash(const passgen_hashmap *map, const void *key, bool first) {
    UNUSED(map);
    uint64_t output;
    const struct siphash_keys *keys = map->context_data;
    const char *siphash_key = first ? keys->first : keys->second;
    passgen_siphash(
        key,
        utf32_len(key),
        siphash_key,
        (uint8_t *) &output,
        sizeof(output));
    return output;
}

static bool
utf32_equal(const passgen_hashmap *map, const void *lhs, const void *rhs) {
    UNUSED(map);
    // compute lengths of lhs and rhs
    size_t rhs_len = utf32_len(rhs);
    size_t lhs_len = utf32_len(lhs);

    // if lengths are not the same, the keys cannot be equal
    if(rhs_len != lhs_len) {
        return false;
    }

    // compare lhs and rhs
    int ret = memcmp(lhs, rhs, rhs_len * sizeof(uint32_t));

    // return true if comparison was equal
    return ret == 0;
}

static void utf32_init(passgen_hashmap *map) {
    map->context_data = (void *) &utf32_keys;
}

static void utf32_fini(passgen_hashmap *map) {
    map->context_data = NULL;
}

const passgen_hashmap_context passgen_hashmap_context_utf32 = {
    .hash = utf32_hash,
    .equal = utf32_equal,
    .init = utf32_init,
    .fini = utf32_fini,
};

int passgen_hashmap_entry_free(void *user, passgen_hashmap_entry *entry) {
    (void) user;
    free((void *) entry->key);
    free(entry->value);
    PASSGEN_CLEAR(entry);
    return 0;
}
