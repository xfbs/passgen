#include "passgen/container/hashmap.h"
#include "passgen/util/siphash.h"
#include "passgen/config.h"
#include <stdlib.h>
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
    free(map->data);

    // set everything to zero
    PASSGEN_CLEAR(map);
}

void passgen_hashmap_realloc(passgen_hashmap *map, size_t capacity) {
    // save previous hashmap
    passgen_hashmap old = *map;

    map->data = calloc(capacity, sizeof(passgen_hashmap_entry));
    map->capacity = capacity;
    map->len = 0;

    // copy data over
    for(size_t i = 0; i < old.capacity; i++) {
        if(old.data[i].key) {
            passgen_hashmap_insert(map, old.data[i].key, old.data[i].value);
        }
    }

    free(old.data);
}

static inline size_t passgen_hashmap_position(
    const passgen_hashmap *map,
    const void *key,
    bool first) {
    return map->context->hash(map, key, first) % map->capacity;
}

static inline bool passgen_hashmap_move(passgen_hashmap *map, size_t pos) {
    const void *key = map->data[pos].key;
    bool first = passgen_hashmap_position(map, key, true) == pos;
    size_t other_pos = passgen_hashmap_position(map, key, !first);
    if(!map->data[other_pos].key) {
        map->data[other_pos].key = map->data[pos].key;
        map->data[other_pos].value = map->data[pos].value;
        return true;
    }
    return false;
}

static inline size_t next_capacity(passgen_hashmap *map) {
    size_t i;
    for(i = 0; hashmap_sizes[i]; i++) {
        if(hashmap_sizes[i] >= map->capacity) {
            break;
        }
    }
    if(hashmap_sizes[i] && hashmap_sizes[i + 1]) {
        return hashmap_sizes[i + 1];
    } else {
        return 2 * map->capacity - 1;
    }
}

passgen_hashmap_entry
passgen_hashmap_insert(passgen_hashmap *map, const void *key, void *value) {
    if(map->capacity == 0) {
        passgen_hashmap_realloc(map, hashmap_sizes[0]);
    }

    size_t hash;

    hash = passgen_hashmap_position(map, key, true);
    if(!map->data[hash].key) {
        map->data[hash].key = key;
        map->data[hash].value = value;
        map->len += 1;
        return (passgen_hashmap_entry){NULL, NULL};
    } else {
        if(map->context->equal(map, map->data[hash].key, key)) {
            passgen_hashmap_entry entry = map->data[hash];
            map->data[hash].key = key;
            map->data[hash].value = value;
            return entry;
        } else if(passgen_hashmap_move(map, hash)) {
            map->data[hash].key = key;
            map->data[hash].value = value;
            map->len += 1;
            return (passgen_hashmap_entry){NULL, NULL};
        }
    }

    hash = passgen_hashmap_position(map, key, false);
    if(!map->data[hash].key) {
        map->data[hash].key = key;
        map->data[hash].value = value;
        map->len += 1;
        return (passgen_hashmap_entry){NULL, NULL};
    } else {
        if(map->context->equal(map, map->data[hash].key, key)) {
            passgen_hashmap_entry entry = map->data[hash];
            map->data[hash].key = key;
            map->data[hash].value = value;
            return entry;
        } else if(passgen_hashmap_move(map, hash)) {
            map->data[hash].key = key;
            map->data[hash].value = value;
            map->len += 1;
            return (passgen_hashmap_entry){NULL, NULL};
        }
    }

    passgen_hashmap_realloc(map, next_capacity(map));
    return passgen_hashmap_insert(map, key, value);
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
