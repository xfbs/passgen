#include "passgen/assert.h"
#include "passgen/config.h"
#include "passgen/monocypher.h"
#include "passgen/random.h"
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
    free(context);
}

passgen_random *passgen_random_chacha20_open(
    passgen_random *random,
    const uint8_t key[CHACHA20_KEY_SIZE],
    const uint8_t iv[8]) {
    passgen_random_chacha20_context *data =
        malloc(sizeof(passgen_random_chacha20_context));
    if(!data) {
        return NULL;
    }

    if(!random) {
        random = malloc(sizeof(passgen_random));
        if(!random) {
            free(data);
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

    passgen_random_reload(random);

    return random;
}

void passgen_random_chacha20_seek(passgen_random *random, size_t position) {
    // make sure this really is a chacha20 randomness source
    passgen_assert(random->read == passgen_random_chacha20_read);
    passgen_assert(random->close == passgen_random_chacha20_close);

    // reset offset
    passgen_random_chacha20_context *data = random->context;
    data->offset = position;

    // reload random data buffer
    passgen_random_reload(random);
}

const passgen_argon2_config PASSGEN_ARGON2_CONFIG_DEFAULT = {
    .algorithm = CRYPTO_ARGON2_ID,
    .nb_blocks = 8,
    .nb_passes = 8,
    .nb_lanes = 1,
};

passgen_random *passgen_random_chacha20_argon2_open(
    passgen_random *random,
    const uint8_t *passphrase,
    const uint8_t *domain,
    const uint8_t *token,
    passgen_argon2_config *config_ptr) {
    static const uint8_t salt[] = "passgen";
    passgen_argon2_inputs inputs = {
        .pass = passphrase,
        .pass_size = strlen((const char *) passphrase),
        .salt = salt,
        .salt_size = sizeof(salt) - 1,
    };

    passgen_argon2_extras extras = {
        .key = domain,
        .key_size = domain ? strlen((const char *) domain) : 0,
        .ad = token,
        .ad_size = token ? strlen((const char *) token) : 0,
    };

    passgen_argon2_config config = PASSGEN_ARGON2_CONFIG_DEFAULT;
    if(config_ptr) {
        config = *config_ptr;
    }

    uint8_t hash[CHACHA20_KEY_SIZE];
    void *work_area = malloc((size_t) 1024 * config.nb_blocks);
    if(!work_area) {
        return NULL;
    }
    passgen_argon2(&hash[0], sizeof(hash), work_area, config, inputs, extras);
    free(work_area);

    static const uint8_t iv[8] = "passgen";
    random = passgen_random_chacha20_open(random, hash, iv);

    // don't leave the derived key lying around on the stack.
    passgen_wipe(hash, sizeof(hash));

    return random;
}
