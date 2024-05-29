#include "passgen/random.h"
#include "passgen/util/endian.h"
#include <stdlib.h>
#include <string.h>

static uint64_t xorshift64(uint64_t *state) {
    uint64_t x = *state;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    return *state = x;
}

static size_t
passgen_random_read_xorshift(void *context, void *dest, size_t size) {
    size_t written = 0;
    uint64_t result;

    // fill all whole uint64 blocks
    while((size - written) >= sizeof(result)) {
        result = xorshift64(context);
        memcpy(dest + written, &result, sizeof(result));
        written += sizeof(result);
    }

    // maybe fill the last incomplete block
    if(size != written) {
        result = xorshift64(context);
        memcpy(dest + written, &result, size - written);
        written += size - written;
    }

    return written;
}

static void passgen_random_close_xorshift(void *context) {
    free(context);
}

passgen_random *
passgen_random_open_xorshift(passgen_random *random, uint64_t seed) {
    if(!random) {
        random = malloc(sizeof(passgen_random));
        if(!random) return NULL;
    }

    // create state
    uint64_t *state = malloc(sizeof(uint64_t));
    if(!state) return NULL;

    // initialise state
    *state = seed;

    random->data = state;
    random->read = passgen_random_read_xorshift;
    random->close = passgen_random_close_xorshift;
    passgen_random_reload(random);

    return random;
}
