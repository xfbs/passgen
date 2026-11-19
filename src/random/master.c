#include "passgen/master.h"
#include "passgen/monocypher.h"
#include "passgen/util/alloc.h"
#include "passgen/util/clear.h"
#include <string.h>

// the costs of the levels of version 1. these must never change, as that
// would change every passphrase derived from a master passphrase. other
// costs get a new version.
static const passgen_master_cost master_costs_v1[PASSGEN_MASTER_LEVEL_MAX] = {
    {.memory = 64 * 1024, .passes = 3},
    {.memory = 256 * 1024, .passes = 3},
    {.memory = 1024 * 1024, .passes = 3},
};

bool passgen_master_level(
    unsigned version,
    unsigned level,
    passgen_master_cost *cost) {
    if(version != 1 || level < 1 || level > PASSGEN_MASTER_LEVEL_MAX) {
        return false;
    }
    *cost = master_costs_v1[level - 1];
    return true;
}

passgen_status passgen_master_derive(
    passgen_master *master,
    unsigned version,
    unsigned level,
    const char *passphrase,
    const uint8_t *seed,
    size_t seed_size) {
    passgen_master_cost cost;
    if(!passgen_master_level(version, level, &cost)) {
        return PASSGEN_ERROR_INVALID_ARGUMENT;
    }
    return passgen_master_derive_cost(
        master,
        version,
        &cost,
        passphrase,
        seed,
        seed_size);
}

passgen_status passgen_master_derive_cost(
    passgen_master *master,
    unsigned version,
    const passgen_master_cost *cost,
    const char *passphrase,
    const uint8_t *seed,
    size_t seed_size) {
    // Monocypher does not check the cost, and what it does for one which is
    // not valid is undefined. it needs at least 8 blocks for one lane.
    if(version != 1 || seed_size > PASSGEN_MASTER_SEED_MAX ||
       cost->memory < 8 || cost->passes < 1) {
        return PASSGEN_ERROR_INVALID_ARGUMENT;
    }

    // 16 bytes, which is what is recommended. it is fixed, so that nothing
    // has to be stored, the seed is what can be.
    static const uint8_t salt[] = "passgen/master/1";
    passgen_argon2_inputs inputs = {
        .pass = (const uint8_t *) passphrase,
        .pass_size = strlen(passphrase),
        .salt = salt,
        .salt_size = sizeof(salt) - 1,
    };
    passgen_argon2_extras extras = {
        .key = seed_size ? seed : NULL,
        .key_size = seed_size,
        .ad = NULL,
        .ad_size = 0,
    };
    passgen_argon2_config config = {
        .algorithm = CRYPTO_ARGON2_ID,
        .nb_blocks = cost->memory,
        .nb_passes = cost->passes,
        .nb_lanes = 1,
    };

    // this is wiped by Monocypher once it is done.
    size_t work_area_size = (size_t) 1024 * config.nb_blocks;
    void *work_area = passgen_alloc(work_area_size);
    if(!work_area) {
        return PASSGEN_ERROR_OUT_OF_MEMORY;
    }
    master->version = version;
    passgen_argon2(
        master->key,
        sizeof(master->key),
        work_area,
        config,
        inputs,
        extras);
    passgen_free(work_area, work_area_size);

    return PASSGEN_OK;
}

void passgen_master_hash(
    const passgen_master *master,
    uint8_t hash[PASSGEN_MASTER_HASH_SIZE]) {
    static const uint8_t message[] = "passgen/hash";
    passgen_blake2b_keyed(
        hash,
        PASSGEN_MASTER_HASH_SIZE,
        master->key,
        sizeof(master->key),
        message,
        sizeof(message) - 1);
}

// Adds a string to a hash, after its length, so that where one ends and the
// next starts is part of what is hashed.
static void hash_string(passgen_blake2b_ctx *ctx, const char *string) {
    uint64_t length = string ? strlen(string) : 0;
    uint8_t encoded[8];
    for(size_t i = 0; i < 8; i++) {
        encoded[i] = (uint8_t) (length >> (8 * i));
    }
    passgen_blake2b_update(ctx, encoded, sizeof(encoded));
    if(length) {
        passgen_blake2b_update(ctx, (const uint8_t *) string, length);
    }
}

passgen_random *passgen_master_random_open(
    passgen_random *random,
    const passgen_master *master,
    const char *domain,
    const char *user) {
    static const uint8_t prefix[] = "passgen/site";
    passgen_blake2b_ctx ctx;
    passgen_blake2b_keyed_init(
        &ctx,
        PASSGEN_MASTER_KEY_SIZE,
        master->key,
        sizeof(master->key));
    passgen_blake2b_update(&ctx, prefix, sizeof(prefix) - 1);
    hash_string(&ctx, domain);
    hash_string(&ctx, user);

    uint8_t key[PASSGEN_MASTER_KEY_SIZE];
    // this wipes the context.
    passgen_blake2b_final(&ctx, key);

    static const uint8_t iv[8] = "passgen";
    random = passgen_random_chacha20_open(random, key, iv);
    passgen_wipe(key, sizeof(key));
    return random;
}

void passgen_master_wipe(passgen_master *master) {
    PASSGEN_CLEAR(master);
}
