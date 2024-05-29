#include "passgen/random.h"
#include "passgen/assert.h"
#include "passgen/util/endian.h"
#include <stdlib.h>
#include <string.h>

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
        random->read(&random->buffer, sizeof(random->buffer), random->data);
    (void) bytes;

    // make sure we've read enough.
    passgen_assert(bytes == sizeof(random->buffer));

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
            memcpy(data, random->buffer, left);
            passgen_random_reload(random);

            // if there's more to be read, recurse.
            if(bytes != left) {
                passgen_random_read(random, data + left, bytes - left);
            }
        }
    } else {
        size_t ret = random->read(data, bytes, random->data);
        if(ret != bytes) {
            fprintf(
                stderr,
                "Error reading from randomness source: trying to read %zu "
                "bytes but got %zu",
                bytes,
                ret);
            abort();
        }
    }
}

passgen_random *
passgen_random_open_parse(passgen_random *random, const char *desc) {
    if(strcmp("zero", desc) == 0) {
        return passgen_random_open_zero(random);
    }

    // check if we should read randomness from this file
    if(_strprefix("file:", desc)) {
        return passgen_random_open_path(random, &desc[5]);
    }

    // check if we should use the xorshift PRNG with the given seed
    if(_strprefix("xorshift:", desc)) {
        const char *seed_str = &desc[9];
        if(*seed_str == 0) {
            return NULL;
        }

        uint64_t seed = atoll(seed_str);
        if(seed == 0) {
            return NULL;
        }

        return passgen_random_open_xorshift(random, seed);
    }

    // check if we should use the system default
    if(0 == strcmp(desc, "system")) {
        return passgen_random_open_system(random);
    }

    return NULL;
}

passgen_random *passgen_random_open(passgen_random *random, const char *desc) {
    if(desc) {
        return passgen_random_open_parse(random, desc);
    }

    return passgen_random_open_system(random);
}

void passgen_random_close(passgen_random *random) {
    // close randomness source
    random->close(random->data);

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

    uint8_t mask = max;
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

    uint16_t mask = max;
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

    uint32_t mask = max;
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

    uint64_t mask = max;
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
