#include "passgen/random.h"
#include "passgen/assert.h"
#include "passgen/config.h"
#include "passgen/util/endian.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "passgen/util/clear.h"

static bool _strprefix(const char *prefix, const char *string) {
    size_t prefix_len = strlen(prefix);
    for(size_t i = 0; i < prefix_len; i++) {
        if(prefix[i] != string[i]) {
            return false;
        }
    }
    return true;
}

void passgen_random_reload(passgen_random *random) {
    passgen_assert(random != NULL);

    // read random data.
    size_t bytes =
        random->read(random->context, &random->buffer, sizeof(random->buffer));

    // make sure we've read enough. this must be a hard failure even in
    // release builds: continuing with a partially filled buffer would mean
    // generating output from stale or uninitialized data.
    if(bytes != sizeof(random->buffer)) {
        fprintf(
            stderr,
            "Error reading from randomness source: trying to read %zu "
            "bytes but got %zu\n",
            sizeof(random->buffer),
            bytes);
        abort();
    }

    // reset position in ring buffer.
    random->pos = 0;
}

void passgen_random_read(passgen_random *random, void *data, size_t bytes) {
    if(bytes <= sizeof(random->buffer)) {
        // maximum bytes we can have right now
        size_t left = sizeof(random->buffer) - random->pos;

        if(bytes < left) {
            // if we have enough, just copy it over.
            memcpy(data, &random->buffer[random->pos], bytes);
            random->pos += bytes;
        } else {
            // if we don't have enough, copy over whatever we have and
            // reload.
            memcpy(data, &random->buffer[random->pos], left);
            passgen_random_reload(random);

            // if there's more to be read, recurse.
            if(bytes != left) {
                passgen_random_read(
                    random,
                    (uint8_t *) data + left,
                    bytes - left);
            }
        }
    } else {
        size_t ret = random->read(random->context, data, bytes);
        if(ret != bytes) {
            fprintf(
                stderr,
                "Error reading from randomness source: trying to read %zu "
                "bytes but got %zu\n",
                bytes,
                ret);
            abort();
        }
    }
}

passgen_random *
passgen_random_open_parse(passgen_random *random, const char *desc) {
    if(strcmp("zero", desc) == 0) {
        return passgen_random_zero_open(random);
    }

    // check if we should read randomness from this file
    if(_strprefix("file:", desc)) {
        return passgen_random_path_open(random, &desc[5]);
    }

    // check if we should use the xorshift PRNG with the given seed
    if(_strprefix("xorshift:", desc)) {
        const char *seed_str = &desc[9];
        if(*seed_str == 0) {
            return NULL;
        }

        // the seed is any nonzero 64 bit number, so this needs to parse the
        // full unsigned range, and only accept digits (strtoull() would
        // accept a sign and leading whitespace as well).
        if(*seed_str < '0' || *seed_str > '9') {
            return NULL;
        }

        char *seed_end = NULL;
        errno = 0;
        unsigned long long seed = strtoull(seed_str, &seed_end, 10);
        if(errno != 0 || *seed_end != 0 || seed == 0 || seed > UINT64_MAX) {
            return NULL;
        }

        return passgen_random_xorshift_open(random, seed);
    }

    // check if we should use the system default
    if(0 == strcmp(desc, "system")) {
        return passgen_random_system_open(random);
    }

#ifdef PASSGEN_MONOCYPHER
    // use chacha20 with raw key and IV
    if(_strprefix("chacha20:", desc)) {
        const char *key_ptr = &desc[9];
        const char *key_end = strchr(key_ptr, ':');
        if(!key_end) {
            key_end = key_ptr + strlen(key_ptr);
        }
        uint8_t key[32] = {0};
        size_t key_len = key_end - key_ptr;
        if(key_len > sizeof(key)) {
            return NULL;
        }
        memcpy(key, key_ptr, key_len);

        uint8_t iv[8] = "passgen";

        if(*key_end == ':') {
            const char *iv_ptr = key_end + 1;
            size_t iv_len = strlen(iv_ptr);
            iv_len = iv_len > 8 ? 8 : iv_len;
            memset(iv, 0, 8);
            memcpy(iv, iv_ptr, iv_len);
        }

        return passgen_random_chacha20_open(random, key, iv);
    }

    // use chacha20 with raw key and IV
    if(_strprefix("chacha20-argon2:", desc)) {
        // one extra byte so that these are always NUL-terminated.
        char master_pass[129] = {0};
        char domain[129] = {0};
        char token[129] = {0};

        const char *start = &desc[16];
        const char *end = &desc[16];

#define PARSE_STRING(name) \
        if(*end == ':') end++; \
        if(*end) { \
            start = end; \
            end = strchr(start, ':'); \
            if(!end) { \
                end = start + strlen(start); \
            } \
            if((size_t) (end - start) >= sizeof(name)) { \
                return NULL; \
            } \
            memcpy(name, start, end - start); \
        }

        PARSE_STRING(master_pass)
        PARSE_STRING(domain)
        PARSE_STRING(token)
#undef PARSE_STRING

        return passgen_random_chacha20_argon2_open(
            random,
            master_pass,
            domain,
            token,
            NULL);
    }
#endif

    return NULL;
}

passgen_random *passgen_random_open(passgen_random *random, const char *desc) {
    if(desc) {
        return passgen_random_open_parse(random, desc);
    }

    return passgen_random_system_open(random);
}

void passgen_random_close(passgen_random *random) {
    // close randomness source
    random->close(random->context);

    // reset members to prevent accidental use-after-free
    PASSGEN_CLEAR(random);
}

void passgen_random_free(passgen_random *random) {
    passgen_random_close(random);
    free(random);
}

inline uint8_t passgen_random_u8(passgen_random *random) {
    uint8_t data;
    passgen_random_read(random, &data, sizeof(data));
    return data;
}

inline uint16_t passgen_random_u16(passgen_random *random) {
    uint16_t data;
    passgen_random_read(random, &data, sizeof(data));
    data = htole16(data);
    return data;
}

inline uint32_t passgen_random_u32(passgen_random *random) {
    uint32_t data;
    passgen_random_read(random, &data, sizeof(data));
    data = htole32(data);
    return data;
}

inline uint64_t passgen_random_u64(passgen_random *random) {
    uint64_t data;
    passgen_random_read(random, &data, sizeof(data));
    data = htole64(data);
    return data;
}

inline bool passgen_random_bool(passgen_random *random) {
    uint8_t data;
    passgen_random_read(random, &data, sizeof(data));
    return (data & 128) == 0;
}

inline uint8_t passgen_random_u8_max(passgen_random *random, uint8_t max) {
    passgen_assert(max);

    // smallest mask of the form 2^n - 1 that covers the largest value we may
    // return, which is max - 1.
    uint8_t mask = max - 1;
    mask |= mask >> 4;
    mask |= mask >> 2;
    mask |= mask >> 1;

    uint8_t num;
    do {
        num = passgen_random_u8(random) & mask;
    } while(num >= max);

    return num;
}

inline uint16_t passgen_random_u16_max(passgen_random *random, uint16_t max) {
    passgen_assert(max);

    // smallest mask of the form 2^n - 1 that covers the largest value we may
    // return, which is max - 1.
    uint16_t mask = max - 1;
    mask |= mask >> 8;
    mask |= mask >> 4;
    mask |= mask >> 2;
    mask |= mask >> 1;

    uint16_t num;

    do {
        num = passgen_random_u16(random) & mask;
    } while(num >= max);

    return num;
}

inline uint32_t passgen_random_u32_max(passgen_random *random, uint32_t max) {
    passgen_assert(max);

    // smallest mask of the form 2^n - 1 that covers the largest value we may
    // return, which is max - 1.
    uint32_t mask = max - 1;
    mask |= mask >> 16;
    mask |= mask >> 8;
    mask |= mask >> 4;
    mask |= mask >> 2;
    mask |= mask >> 1;

    uint32_t num;

    do {
        num = passgen_random_u32(random) & mask;
    } while(num >= max);

    return num;
}

inline uint64_t passgen_random_u64_max(passgen_random *random, uint64_t max) {
    passgen_assert(max);

    // smallest mask of the form 2^n - 1 that covers the largest value we may
    // return, which is max - 1.
    uint64_t mask = max - 1;
    mask |= mask >> 32;
    mask |= mask >> 16;
    mask |= mask >> 8;
    mask |= mask >> 4;
    mask |= mask >> 2;
    mask |= mask >> 1;

    uint64_t num;

    do {
        num = passgen_random_u64(random) & mask;
    } while(num >= max);

    return num;
}
