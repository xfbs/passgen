#include "passgen/random.h"
#include "passgen/assert.h"
#include <stdlib.h>
#include <string.h>

#ifdef __linux__
#    define PASSGEN_RANDOM_HAVE_SYSTEM
#    include <sys/random.h>

size_t passgen_random_read_system(void *dest, size_t size, void *data) {
    (void) data;
    return getrandom(dest, size, 0);
}
#endif

#ifdef __APPLE__
#    define PASSGEN_RANDOM_HAVE_SYSTEM

size_t passgen_random_read_system(void *dest, size_t size, void *data) {
    (void) data;
    arc4random_buf(dest, size);
    return size;
}
#endif

#ifndef PASSGEN_RANDOM_HAVE_SYSTEM
static const char *passgen_random_default_device = "/dev/urandom";
#endif

size_t passgen_random_read_file(void *dest, size_t size, void *data) {
    return fread(dest, 1, size, data);
}

void passgen_random_close_file(void *data) {
    fclose(data);
}

void passgen_random_close_system(void *data) {
    (void) data;
}

static uint64_t xorshift64(uint64_t *state) {
    uint64_t x = *state;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    return *state = x;
}

size_t passgen_random_read_xorshift(void *dest, size_t size, void *data) {
    size_t written = 0;
    uint64_t result;

    // fill all whole uint64 blocks
    while((size - written) >= sizeof(result)) {
        result = xorshift64(data);
        memcpy(dest + written, &result, sizeof(result));
        written += sizeof(result);
    }

    // maybe fill the last incomplete block
    if(size != written) {
        result = xorshift64(data);
        memcpy(dest + written, &result, size - written);
        written += size - written;
    }

    return written;
}

void passgen_random_close_xorshift(void *data) {
    free(data);
}

passgen_random_t *passgen_random_new_xorshift(uint64_t seed) {
    passgen_random_t *random = malloc(sizeof(passgen_random_t));
    if(!random) return NULL;

    return passgen_random_open_xorshift(random, seed);
}

void passgen_random_reload(passgen_random_t *random) {
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

passgen_random_t *
passgen_random_open_xorshift(passgen_random_t *random, uint64_t seed) {
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

#ifdef PASSGEN_RANDOM_HAVE_SYSTEM
passgen_random_t *passgen_random_open_system(passgen_random_t *random) {
    random->data = NULL;
    random->read = passgen_random_read_system;
    random->close = passgen_random_close_system;
    passgen_random_reload(random);
    return random;
}
#else
passgen_random_t *passgen_random_open_system(passgen_random_t *random) {
    (void) random;

    return NULL;
}
#endif

void passgen_random_read(passgen_random_t *random, void *data, size_t bytes) {
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
        int ret = random->read(data, bytes, random->data);
        passgen_assert(ret == bytes);
    }
}

passgen_random_t *passgen_random_new() {
    passgen_random_t *random = malloc(sizeof(passgen_random_t));
    passgen_assert(random);

    return passgen_random_open(random);
}

passgen_random_t *passgen_random_new_path(const char *path) {
    passgen_random_t *random = malloc(sizeof(passgen_random_t));
    if(!random) return NULL;

    if(!passgen_random_open_path(random, path)) {
        free(random);
        return NULL;
    }

    return random;
}

passgen_random_t *passgen_random_new_file(FILE *file) {
    passgen_random_t *random = malloc(sizeof(passgen_random_t));
    if(!random) return NULL;

    passgen_random_open_file(random, file);

    return random;
}

passgen_random_t *passgen_random_open(passgen_random_t *random) {
#ifdef PASSGEN_RANDOM_HAVE_SYSTEM
    return passgen_random_open_system(random);
#else
    return passgen_random_open_path(random, passgen_random_default_device);
#endif
}

passgen_random_t *
passgen_random_open_path(passgen_random_t *random, const char *path) {
    FILE *device = fopen(path, "r");
    if(!device) return NULL;

    return passgen_random_open_file(random, device);

    passgen_random_reload(random);

    return random;
}

passgen_random_t *
passgen_random_open_file(passgen_random_t *random, FILE *file) {
    random->data = file;
    random->read = passgen_random_read_file;
    random->close = passgen_random_close_file;
    passgen_random_reload(random);
    return random;
}

void passgen_random_close(passgen_random_t *random) {
    if(random != NULL) {
        random->close(random->data);
        random->data = NULL;
        random->read = NULL;
        random->close = NULL;
    }
}

void passgen_random_free(passgen_random_t *random) {
    passgen_random_close(random);
    free(random);
}

inline uint8_t passgen_random_u8(passgen_random_t *random) {
    uint8_t data;
    passgen_random_read(random, &data, sizeof(data));
    return data;
}

inline uint16_t passgen_random_u16(passgen_random_t *random) {
    uint16_t data;
    passgen_random_read(random, &data, sizeof(data));
    return data;
}

inline uint32_t passgen_random_u32(passgen_random_t *random) {
    uint32_t data;
    passgen_random_read(random, &data, sizeof(data));
    return data;
}

inline uint64_t passgen_random_u64(passgen_random_t *random) {
    uint64_t data;
    passgen_random_read(random, &data, sizeof(data));
    return data;
}

inline bool passgen_random_bool(passgen_random_t *random) {
    uint8_t data;
    passgen_random_read(random, &data, sizeof(data));
    return (data & 128) == 0;
}

inline uint8_t passgen_random_u8_max(passgen_random_t *random, uint8_t max) {
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

inline uint16_t passgen_random_u16_max(passgen_random_t *random, uint16_t max) {
    passgen_assert(max);

    if(max <= UINT8_MAX) {
        return passgen_random_u8_max(random, max);
    }

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

inline uint32_t passgen_random_u32_max(passgen_random_t *random, uint32_t max) {
    passgen_assert(max);

    if(max < UINT16_MAX) {
        return passgen_random_u16_max(random, max);
    }

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

inline uint64_t passgen_random_u64_max(passgen_random_t *random, uint64_t max) {
    passgen_assert(max);

    if(max < UINT32_MAX) {
        return passgen_random_u32_max(random, max);
    }

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
