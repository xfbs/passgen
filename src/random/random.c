#include "passgen/random.h"
#include "passgen/random_desc.h"
#include "passgen/util/alloc.h"
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

// Called when the source did not have as much data as was asked for. From
// here on, the source only gives out zeroes, and says that it has failed.
static void passgen_random_fail(passgen_random *random) {
    random->failed = true;
    memset(random->buffer, 0, sizeof(random->buffer));
    random->pos = 0;
}

bool passgen_random_failed(const passgen_random *random) {
    return random->failed;
}

void passgen_random_reload(passgen_random *random) {
    passgen_assert(random != NULL);

    // once it has failed, it is not asked again.
    if(random->failed) {
        random->pos = 0;
        return;
    }

    // read random data.
    size_t bytes =
        random->read(random->context, &random->buffer, sizeof(random->buffer));

    // make sure we've read enough. what was read is not used: continuing
    // with a partially filled buffer would mean generating output from stale
    // or uninitialized data.
    if(bytes != sizeof(random->buffer)) {
        passgen_random_fail(random);
        return;
    }

    // reset position in ring buffer.
    random->pos = 0;
}

void passgen_random_read(passgen_random *random, void *data, size_t bytes) {
    if(random->failed) {
        memset(data, 0, bytes);
        return;
    }

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

            // if reloading failed, none of this is to be used.
            if(random->failed) {
                memset(data, 0, bytes);
            }
        }
    } else {
        size_t ret = random->read(random->context, data, bytes);
        if(ret != bytes) {
            passgen_random_fail(random);
            memset(data, 0, bytes);
        }
    }
}

#ifdef PASSGEN_MONOCYPHER
// Copies the part of a description up to the next colon (or its end) into a
// string, and moves past it. Returns false if it does not fit.
static bool desc_string(const char **pos, char *string, size_t size) {
    const char *end = strchr(*pos, ':');
    if(!end) {
        end = *pos + strlen(*pos);
    }
    size_t length = (size_t) (end - *pos);
    if(length >= size) {
        return false;
    }
    memcpy(string, *pos, length);
    string[length] = 0;
    *pos = *end ? end + 1 : end;
    return true;
}
#endif

#ifdef PASSGEN_MONOCYPHER
/// Decodes exactly `2 * size` hex digits (in either case) at the start of
/// `string` into `size` bytes. Returns false if there are fewer of them.
static bool hex_decode(const char *string, uint8_t *bytes, size_t size) {
    for(size_t i = 0; i < 2 * size; i++) {
        char c = string[i];
        uint8_t digit;
        if(c >= '0' && c <= '9') {
            digit = c - '0';
        } else if(c >= 'a' && c <= 'f') {
            digit = c - 'a' + 10;
        } else if(c >= 'A' && c <= 'F') {
            digit = c - 'A' + 10;
        } else {
            return false;
        }
        if(i % 2 == 0) {
            bytes[i / 2] = digit << 4;
        } else {
            bytes[i / 2] |= digit;
        }
    }
    return true;
}
#endif

/// What every kind of description starts with. The ones without parameters
/// are the whole description, all others are followed by their parameters.
static const struct {
    const char *prefix;
    bool whole;
    passgen_random_desc_kind kind;
} random_desc_kinds[] = {
    {"zero", true, PASSGEN_RANDOM_DESC_ZERO},
    {"system", true, PASSGEN_RANDOM_DESC_SYSTEM},
    {"file:", false, PASSGEN_RANDOM_DESC_FILE},
    {"xorshift:", false, PASSGEN_RANDOM_DESC_XORSHIFT},
    {"chacha20:", false, PASSGEN_RANDOM_DESC_CHACHA20},
    {"chacha20-argon2:", false, PASSGEN_RANDOM_DESC_CHACHA20_ARGON2},
};

/// Finds out which kind of source a description is for, from how it starts,
/// whether or not the rest of it is valid. Sets `params` to what follows the
/// prefix. Returns false if it is not any known kind.
static bool random_desc_kind(
    const char *string,
    passgen_random_desc_kind *kind,
    const char **params) {
    size_t count = sizeof(random_desc_kinds) / sizeof(random_desc_kinds[0]);
    for(size_t i = 0; i < count; i++) {
        const char *prefix = random_desc_kinds[i].prefix;
        bool matches = random_desc_kinds[i].whole
                           ? 0 == strcmp(prefix, string)
                           : _strprefix(prefix, string);
        if(matches) {
            *kind = random_desc_kinds[i].kind;
            *params = &string[strlen(prefix)];
            return true;
        }
    }
    return false;
}

bool passgen_random_desc_secret(const char *string) {
    passgen_random_desc_kind kind;
    const char *params;
    // an unknown kind may be a mistyped one which holds a secret.
    if(!random_desc_kind(string, &kind, &params)) {
        return true;
    }

    // there is no default case, so that a new kind is a warning here until
    // it is decided whether it holds a secret.
    switch(kind) {
        case PASSGEN_RANDOM_DESC_ZERO:
        case PASSGEN_RANDOM_DESC_SYSTEM:
        case PASSGEN_RANDOM_DESC_FILE:
        case PASSGEN_RANDOM_DESC_XORSHIFT:
            return false;
        case PASSGEN_RANDOM_DESC_CHACHA20:
        case PASSGEN_RANDOM_DESC_CHACHA20_ARGON2:
            return true;
    }
    return true;
}

bool passgen_random_desc_parse(passgen_random_desc *desc, const char *string) {
    memset(desc, 0, sizeof(*desc));

    passgen_random_desc_kind kind;
    const char *params;
    if(!random_desc_kind(string, &kind, &params)) {
        return false;
    }

    switch(kind) {
        case PASSGEN_RANDOM_DESC_ZERO:
        case PASSGEN_RANDOM_DESC_SYSTEM:
            break;

        // read randomness from this file
        case PASSGEN_RANDOM_DESC_FILE:
            desc->data.file.path = params;
            break;

        // use the xorshift PRNG with the given seed
        case PASSGEN_RANDOM_DESC_XORSHIFT: {
            // the seed is any nonzero 64 bit number, so this needs to parse
            // the full unsigned range, and only accept digits (strtoull()
            // would accept a sign and leading whitespace as well).
            if(*params < '0' || *params > '9') {
                return false;
            }

            char *seed_end = NULL;
            errno = 0;
            unsigned long long seed = strtoull(params, &seed_end, 10);
            if(errno != 0 || *seed_end != 0 || seed == 0 || seed > UINT64_MAX) {
                return false;
            }
            desc->data.xorshift.seed = seed;
            break;
        }

        // use chacha20 with a key and an IV in hex: exactly 64 hex digits for
        // the 32 bytes of the key, and if there is an IV, exactly 16 for its
        // 8 bytes.
        case PASSGEN_RANDOM_DESC_CHACHA20: {
#ifdef PASSGEN_MONOCYPHER
            const char *pos = params;
            uint8_t *key = desc->data.chacha20.key;
            uint8_t *iv = desc->data.chacha20.iv;
            if(!hex_decode(pos, key, 32)) {
                PASSGEN_CLEAR(desc);
                return false;
            }
            pos += 64;

            memcpy(iv, "passgen", 8);
            if(*pos == ':') {
                pos++;
                if(!hex_decode(pos, iv, 8)) {
                    PASSGEN_CLEAR(desc);
                    return false;
                }
                pos += 16;
            }

            // nothing may follow, so that a key or IV which is too long is
            // not silently cut off.
            if(*pos) {
                PASSGEN_CLEAR(desc);
                return false;
            }
            break;
#else
            return false;
#endif
        }

        // use chacha20 with a key derived from a master passphrase
        case PASSGEN_RANDOM_DESC_CHACHA20_ARGON2: {
#ifdef PASSGEN_MONOCYPHER
            // one colon before the master passphrase is skipped, so that
            // `chacha20-argon2::x` has the master passphrase `x`. this is how
            // it always was, and changing it would change what is derived
            // from it.
            const char *pos = params;
            if(*pos == ':') {
                pos++;
            }
            char *passphrase = desc->data.chacha20_argon2.passphrase;
            char *domain = desc->data.chacha20_argon2.domain;
            char *token = desc->data.chacha20_argon2.token;
            size_t size = PASSGEN_RANDOM_DESC_STRING_MAX + 1;
            if(!desc_string(&pos, passphrase, size) ||
               !desc_string(&pos, domain, size) ||
               !desc_string(&pos, token, size)) {
                PASSGEN_CLEAR(desc);
                return false;
            }
            desc->data.chacha20_argon2.params = NULL;
            break;
#else
            return false;
#endif
        }
    }

    desc->kind = kind;
    return true;
}

passgen_random *passgen_random_desc_open(
    passgen_random *random,
    const passgen_random_desc *desc) {
    switch(desc->kind) {
        case PASSGEN_RANDOM_DESC_ZERO:
            return passgen_random_zero_open(random);
        case PASSGEN_RANDOM_DESC_SYSTEM:
            return passgen_random_system_open(random);
        case PASSGEN_RANDOM_DESC_FILE:
            return passgen_random_path_open(random, desc->data.file.path);
        case PASSGEN_RANDOM_DESC_XORSHIFT:
            return passgen_random_xorshift_open(
                random,
                desc->data.xorshift.seed);
#ifdef PASSGEN_MONOCYPHER
        case PASSGEN_RANDOM_DESC_CHACHA20:
            return passgen_random_chacha20_open(
                random,
                desc->data.chacha20.key,
                desc->data.chacha20.iv);
        case PASSGEN_RANDOM_DESC_CHACHA20_ARGON2:
            return passgen_random_chacha20_argon2_open(
                random,
                desc->data.chacha20_argon2.passphrase,
                desc->data.chacha20_argon2.domain,
                desc->data.chacha20_argon2.token,
                desc->data.chacha20_argon2.params);
#else
        case PASSGEN_RANDOM_DESC_CHACHA20:
        case PASSGEN_RANDOM_DESC_CHACHA20_ARGON2:
            break;
#endif
    }

    return NULL;
}

passgen_random *passgen_random_open(passgen_random *random, const char *desc) {
    if(!desc) {
        return passgen_random_system_open(random);
    }

    passgen_random_desc parsed;
    passgen_random *opened = NULL;
    if(passgen_random_desc_parse(&parsed, desc)) {
        opened = passgen_random_desc_open(random, &parsed);
    }

    // this may have held a master passphrase or a key.
    PASSGEN_CLEAR(&parsed);

    return opened;
}

void passgen_random_close(passgen_random *random) {
    // close randomness source
    random->close(random->context);

    // reset members to prevent accidental use-after-free
    PASSGEN_CLEAR(random);
}

void passgen_random_free(passgen_random *random) {
    passgen_random_close(random);
    passgen_free(random, sizeof(passgen_random));
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
    // with zero, there is nothing to return, and this would loop forever.
    passgen_require(max);

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
    // with zero, there is nothing to return, and this would loop forever.
    passgen_require(max);

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
    // with zero, there is nothing to return, and this would loop forever.
    passgen_require(max);

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
    // with zero, there is nothing to return, and this would loop forever.
    passgen_require(max);

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
