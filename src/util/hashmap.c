#include "passgen/util/hashmap.h"
#include "passgen/util/siphash.h"
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
static const char *SIPHASH_KEY_FIRST = "someverylongpass";
static const char *SIPHASH_KEY_SECOND = "myotherverylongk";

void passgen_hashmap_init(
    passgen_hashmap *map,
    const passgen_hashmap_context *context) {
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
    map->len = 0;

    // copy data over
    for(size_t i = 0; i < old.capacity; i++) {
        if(old.data[i].key) {
            passgen_hashmap_insert(map, old.data[i].key, old.data[i].value);
        }
    }

    free(old.data);
}

static inline size_t
passgen_hashmap_position(passgen_hashmap *map, const void *key, bool first) {
    return map->context->hash(map, key, first) % map->capacity;
}

static inline bool passgen_hashmap_move(passgen_hashmap *map, size_t pos) {
    void *key = map->data[pos].key;
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
        if(map->context->key_equal(map, map->data[hash].key, key)) {
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
        if(map->context->key_equal(map, map->data[hash].key, key)) {
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

passgen_hashmap_entry passgen_hashmap_remove(passgen_hashmap *map, const void *key) {
    size_t hash;

    // if this key exists
    hash = passgen_hashmap_position(map, key, true);
    if(map->data[hash].key) {
        // and it matches
        if(map->context->key_equal(map, map->data[hash].key, key)) {
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
        if(map->context->key_equal(map, map->data[hash].key, key)) {
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
passgen_hashmap_lookup(passgen_hashmap *map, const void *key) {
    // make sure the hashmap is not empty
    if(!map->data) {
        return NULL;
    }

    size_t hash;

    // lookup first possible location
    hash = passgen_hashmap_position(map, key, true);
    if(map->data[hash].key &&
       map->context->key_equal(map, map->data[hash].key, key)) {
        return &map->data[hash];
    }

    // lookup second possible location
    hash = passgen_hashmap_position(map, key, false);
    if(map->data[hash].key &&
       map->context->key_equal(map, map->data[hash].key, key)) {
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

static uint64_t
string_hash(const passgen_hashmap *map, const void *key, bool first) {
    UNUSED(map);
    uint64_t output;
    const char *siphash_key = first ? SIPHASH_KEY_FIRST : SIPHASH_KEY_SECOND;
    passgen_siphash(
        key,
        strlen(key),
        siphash_key,
        (uint8_t *) &output,
        sizeof(output));
    return output;
}

static bool
string_equal(const passgen_hashmap *map, const void *lhs, const void *rhs) {
    UNUSED(map);
    return strcmp(lhs, rhs) == 0;
}

const passgen_hashmap_context passgen_hashmap_context_default = {
    .hash = string_hash,
    .key_equal = string_equal,
};

static size_t unicode_len(const void *data) {
    const int32_t *unicode = data;
    size_t len = 0;
    while(unicode[len]) {
        len++;
    }
    return len;
}

static uint64_t
unicode_hash(const passgen_hashmap *map, const void *key, bool first) {
    UNUSED(map);
    uint64_t output;
    const char *siphash_key = first ? SIPHASH_KEY_FIRST : SIPHASH_KEY_SECOND;
    passgen_siphash(
        key,
        unicode_len(key),
        siphash_key,
        (uint8_t *) &output,
        sizeof(output));
    return output;
}

static bool
unicode_equal(const passgen_hashmap *map, const void *lhs, const void *rhs) {
    UNUSED(map);
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

int passgen_hashmap_entry_free(void *user, passgen_hashmap_entry *entry) {
    (void) user;
    free(entry->key);
    free(entry->value);
    return 0;
}
