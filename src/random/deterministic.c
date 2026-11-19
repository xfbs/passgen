#include "passgen/assert.h"
#include "passgen/config.h"
#include "passgen/monocypher.h"
#include "passgen/random.h"
#include "passgen/util/alloc.h"
#include "passgen/util/clear.h"
#include <stdlib.h>
#include <string.h>

#define CHACHA20_BLOCK_SIZE 64
#define CHACHA20_KEY_SIZE   32
#define MIN(x, y)           ((x) < (y) ? (x) : (y))

typedef struct {
    uint8_t key[CHACHA20_KEY_SIZE];
    uint8_t iv[8];
    size_t offset;
} passgen_random_chacha20_context;

static size_t
passgen_random_chacha20_read_short(void *context_raw, void *dest, size_t size) {
    passgen_assert(size < CHACHA20_BLOCK_SIZE);
    passgen_random_chacha20_context *context = context_raw;
    uint8_t block[CHACHA20_BLOCK_SIZE];
    passgen_chacha20_djb(
        block,
        NULL,
        CHACHA20_BLOCK_SIZE,
        context->key,
        context->iv,
        context->offset / CHACHA20_BLOCK_SIZE);

    memcpy(dest, &block[context->offset % CHACHA20_BLOCK_SIZE], size);
    context->offset += size;
    return size;
}

static size_t
passgen_random_chacha20_read(void *context_raw, void *dest, size_t size) {
    passgen_random_chacha20_context *context = context_raw;
    memset(dest, 0, size);

    size_t offset_bytes = context->offset % CHACHA20_BLOCK_SIZE;
    size_t written = 0;
    if(offset_bytes) {
        size_t short_bytes = MIN(CHACHA20_BLOCK_SIZE - offset_bytes, size);
        passgen_random_chacha20_read_short(context_raw, dest, short_bytes);
        written += short_bytes;
    }

    size_t remaining = size - written;
    passgen_chacha20_djb(
        (uint8_t *) dest + written,
        NULL,
        remaining,
        context->key,
        context->iv,
        context->offset / CHACHA20_BLOCK_SIZE);
    context->offset += remaining;

    return size;
}

static void passgen_random_chacha20_close(void *context_raw) {
    passgen_random_chacha20_context *context = context_raw;
    PASSGEN_CLEAR(context);
    passgen_free(context, sizeof(*context));
}

passgen_random *passgen_random_chacha20_open(
    passgen_random *random,
    const uint8_t key[CHACHA20_KEY_SIZE],
    const uint8_t iv[8]) {
    passgen_random_chacha20_context *data =
        passgen_alloc(sizeof(passgen_random_chacha20_context));
    if(!data) {
        return NULL;
    }

    if(!random) {
        random = passgen_alloc(sizeof(passgen_random));
        if(!random) {
            passgen_free(data, sizeof(*data));
            return NULL;
        }
    }

    memset(data, 0, sizeof(*data));
    memcpy(data->key, key, CHACHA20_KEY_SIZE);
    memcpy(data->iv, iv, 8);
    data->offset = 0;

    random->context = data;
    random->close = passgen_random_chacha20_close;
    random->read = passgen_random_chacha20_read;

    random->failed = false;
    passgen_random_reload(random);

    return random;
}

bool passgen_random_chacha20_seek(passgen_random *random, size_t position) {
    // anything else has another context, which this would write into.
    if(random->read != passgen_random_chacha20_read ||
       random->close != passgen_random_chacha20_close) {
        return false;
    }

    // reset offset
    passgen_random_chacha20_context *data = random->context;
    data->offset = position;

    // reload random data buffer
    passgen_random_reload(random);

    return true;
}
