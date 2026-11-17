#include "passgen/assert.h"
#include "passgen/config.h"
#include "passgen/monocypher.h"
#include "passgen/random.h"
#include <stdlib.h>
#include <string.h>
#include "passgen/util/clear.h"

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

// these must never change, as that would change every passphrase derived
// from a master passphrase. to use other parameters, add a new version.
#define PASSGEN_ARGON2_PARAMS_V1_INIT \
    {                                 \
        .memory = 19 * 1024,          \
        .passes = 2,                  \
        .lanes = 1,                   \
    }

const passgen_argon2_params PASSGEN_ARGON2_PARAMS_V1 =
    PASSGEN_ARGON2_PARAMS_V1_INIT;

const passgen_argon2_params PASSGEN_ARGON2_PARAMS_DEFAULT =
    PASSGEN_ARGON2_PARAMS_V1_INIT;

passgen_random *passgen_random_chacha20_argon2_open(
    passgen_random *random,
    const char *passphrase,
    const char *domain,
    const char *token,
    const passgen_argon2_params *params) {
    // the salt is fixed, as there is nowhere to store one. it is 16 bytes long,
    // which is what is recommended, and other implementations of Argon2 don't
    // accept salts of less than 8 bytes.
    static const uint8_t salt[] = "passgen/argon2id";
    passgen_argon2_inputs inputs = {
        .pass = (const uint8_t *) passphrase,
        .pass_size = strlen(passphrase),
        .salt = salt,
        .salt_size = sizeof(salt) - 1,
    };

    passgen_argon2_extras extras = {
        .key = (const uint8_t *) domain,
        .key_size = domain ? strlen(domain) : 0,
        .ad = (const uint8_t *) token,
        .ad_size = token ? strlen(token) : 0,
    };

    if(!params) {
        params = &PASSGEN_ARGON2_PARAMS_DEFAULT;
    }

    // Monocypher does not check these, and what it does for parameters
    // which are not valid is undefined.
    if(params->lanes < 1 || params->passes < 1 ||
       params->memory / 8 < params->lanes) {
        return NULL;
    }

    passgen_argon2_config config = {
        .algorithm = CRYPTO_ARGON2_ID,
        .nb_blocks = params->memory,
        .nb_passes = params->passes,
        .nb_lanes = params->lanes,
    };

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
