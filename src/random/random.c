#include "passgen/random.h"
#include "passgen/assert.h"
#include "passgen/config.h"
#include "passgen/random_desc.h"
#include "passgen/util/alloc.h"
#include "passgen/util/clear.h"
#include "passgen/util/endian.h"
#include <errno.h>
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

bool passgen_random_stream(passgen_random *random, uint64_t index) {
    if(!random->stream) {
        return false;
    }

    // what is buffered is from the stream it was in.
    random->stream(random->context, index);
    passgen_random_reload(random);
    return true;
}

void passgen_random_read(passgen_random *random, void *data, size_t bytes) {
    // more than fits into the buffer is read directly.
    if(!random->failed && bytes > sizeof(random->buffer)) {
        size_t ret = random->read(random->context, data, bytes);
        if(ret != bytes) {
            passgen_random_fail(random);
        }
    } else {
        // otherwise it comes from the buffer, which is reloaded as soon as
        // all of it was used.
        uint8_t *output = data;
        size_t remaining = bytes;
        while(remaining && !random->failed) {
            size_t left = sizeof(random->buffer) - random->pos;
            size_t count = remaining < left ? remaining : left;
            memcpy(output, &random->buffer[random->pos], count);
            random->pos += count;
            output += count;
            remaining -= count;

            if(random->pos == sizeof(random->buffer)) {
                passgen_random_reload(random);
            }
        }
    }

    // if the source failed (now or before), none of this is to be used.
    if(random->failed) {
        memset(data, 0, bytes);
    }
}

#ifdef PASSGEN_MONOCYPHER
// Copies a field of a description, up to the next colon (or its end), into a
// string, and moves to the next field (past the colon). In it, `\:` is a
// colon and `\\` a backslash, and any other backslash is not valid. Sets
// last to whether it was the last field. Returns false if the field is not
// valid, or it does not fit, with the NUL terminator.
static bool
desc_field(const char **pos, char *string, size_t size, bool *last) {
    const char *current = *pos;
    size_t length = 0;
    while(*current && *current != ':') {
        char c = *current;
        if(c == '\\') {
            current++;
            if(*current != ':' && *current != '\\') {
                return false;
            }
            c = *current;
        }
        if(length + 1 >= size) {
            return false;
        }
        string[length++] = c;
        current++;
    }
    string[length] = 0;
    *last = !*current;
    *pos = *current ? current + 1 : current;
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
    {"master:", false, PASSGEN_RANDOM_DESC_MASTER},
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
        bool matches = random_desc_kinds[i].whole ? 0 == strcmp(prefix, string)
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
        case PASSGEN_RANDOM_DESC_MASTER:
            return true;
    }
    return true;
}

#ifdef PASSGEN_MONOCYPHER
// Parses a number of one to three digits, without leading zeroes.
static bool desc_number(const char *string, unsigned *number) {
    size_t length = strlen(string);
    if(length < 1 || length > 3 || (length > 1 && string[0] == '0')) {
        return false;
    }
    *number = 0;
    for(size_t i = 0; i < length; i++) {
        if(string[i] < '0' || string[i] > '9') {
            return false;
        }
        *number = *number * 10 + (unsigned) (string[i] - '0');
    }
    return true;
}

// Parses what follows `master:`. All six fields have to be there, which
// may be empty except for the version and the level.
static bool random_desc_master(passgen_random_desc *desc, const char *params) {
    char field[2 * PASSGEN_MASTER_SEED_MAX + 1];
    const char *pos = params;
    bool last = false;

    // the version and the level, which have to be known.
    if(!desc_field(&pos, field, sizeof(field), &last) || last ||
       !desc_number(field, &desc->data.master.version)) {
        return false;
    }
    if(!desc_field(&pos, field, sizeof(field), &last) || last ||
       !desc_number(field, &desc->data.master.level)) {
        return false;
    }
    passgen_master_cost cost;
    if(!passgen_master_level(
           desc->data.master.version,
           desc->data.master.level,
           &cost)) {
        return false;
    }

    size_t size = PASSGEN_RANDOM_DESC_STRING_MAX + 1;
    if(!desc_field(&pos, desc->data.master.passphrase, size, &last) || last) {
        return false;
    }

    // the seed, in hex, which may be empty.
    if(!desc_field(&pos, field, sizeof(field), &last) || last) {
        return false;
    }
    size_t digits = strlen(field);
    if(digits % 2 || !hex_decode(field, desc->data.master.seed, digits / 2)) {
        passgen_wipe(field, sizeof(field));
        return false;
    }
    desc->data.master.seed_size = digits / 2;
    passgen_wipe(field, sizeof(field));

    // the user is the last field. a colon after it would be the start of
    // another one, which there is not.
    if(!desc_field(&pos, desc->data.master.domain, size, &last) || last ||
       !desc_field(&pos, desc->data.master.user, size, &last) || !last) {
        return false;
    }

    desc->data.master.cost = NULL;
    return true;
}
#endif

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

        // use chacha20 with a key derived from a master passphrase, as
        // master:<version>:<level>:<passphrase>:<seed>:<domain>:<user>.
        case PASSGEN_RANDOM_DESC_MASTER: {
#ifdef PASSGEN_MONOCYPHER
            if(!random_desc_master(desc, params)) {
                PASSGEN_CLEAR(desc);
                return false;
            }
            break;
#else
            return false;
#endif
        }
    }

    desc->kind = kind;
    return true;
}

#ifdef PASSGEN_MONOCYPHER
// Derives the master key of a description, and opens the source for its
// domain and user.
static passgen_random *random_desc_master_open(
    passgen_random *random,
    const passgen_random_desc *desc) {
    passgen_master master;
    passgen_status status;
    if(desc->data.master.cost) {
        status = passgen_master_derive_cost(
            &master,
            desc->data.master.version,
            desc->data.master.cost,
            desc->data.master.passphrase,
            desc->data.master.seed,
            desc->data.master.seed_size);
    } else {
        status = passgen_master_derive(
            &master,
            desc->data.master.version,
            desc->data.master.level,
            desc->data.master.passphrase,
            desc->data.master.seed,
            desc->data.master.seed_size);
    }
    if(status != PASSGEN_OK) {
        return NULL;
    }

    random = passgen_master_random_open(
        random,
        &master,
        desc->data.master.domain,
        desc->data.master.user);
    passgen_master_wipe(&master);
    return random;
}
#endif

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
        case PASSGEN_RANDOM_DESC_MASTER:
            return random_desc_master_open(random, desc);
#else
        case PASSGEN_RANDOM_DESC_CHACHA20:
        case PASSGEN_RANDOM_DESC_MASTER:
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
