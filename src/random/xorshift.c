#include "passgen/random.h"
#include "passgen/util/alloc.h"
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

typedef struct {
    /// The seed, which the streams are derived from.
    uint64_t seed;
    /// The state of the generator.
    uint64_t state;
} passgen_random_xorshift_context;

static size_t
passgen_random_xorshift_read(void *context_raw, void *dest, size_t size) {
    uint64_t *context =
        &((passgen_random_xorshift_context *) context_raw)->state;
    size_t written = 0;
    uint64_t result;

    // fill all whole uint64 blocks
    while((size - written) >= sizeof(result)) {
        result = xorshift64(context);
        memcpy((uint8_t *) dest + written, &result, sizeof(result));
        written += sizeof(result);
    }

    // maybe fill the last incomplete block
    if(size != written) {
        result = xorshift64(context);
        memcpy((uint8_t *) dest + written, &result, size - written);
        written += size - written;
    }

    return written;
}

// Stream zero is the generator with the seed itself. It cannot jump ahead,
// so the other streams start from a state which mixes the index into the
// seed, with the finalizer of splitmix64. This is the same as the Rust
// implementation does.
static void passgen_random_xorshift_stream(void *context_raw, uint64_t index) {
    passgen_random_xorshift_context *context = context_raw;
    uint64_t mix = index * UINT64_C(0x9e3779b97f4a7c15);
    mix = (mix ^ (mix >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    mix = (mix ^ (mix >> 27)) * UINT64_C(0x94d049bb133111eb);
    mix ^= mix >> 31;

    // the state must never be zero.
    context->state = context->seed ^ mix;
    if(!context->state) {
        context->state = UINT64_MAX;
    }
}

static void passgen_random_xorshift_close(void *context) {
    passgen_free(context, sizeof(passgen_random_xorshift_context));
}

passgen_random *
passgen_random_xorshift_open(passgen_random *random, uint64_t seed) {
    // create state
    passgen_random_xorshift_context *context =
        passgen_alloc(sizeof(passgen_random_xorshift_context));
    if(!context) return NULL;

    if(!random) {
        random = passgen_alloc(sizeof(passgen_random));
        if(!random) {
            passgen_free(context, sizeof(passgen_random_xorshift_context));
            return NULL;
        }
    }

    // initialise state
    context->seed = seed;
    context->state = seed;

    random->context = context;
    random->read = passgen_random_xorshift_read;
    random->close = passgen_random_xorshift_close;
    random->stream = passgen_random_xorshift_stream;
    random->failed = false;
    passgen_random_reload(random);

    return random;
}
