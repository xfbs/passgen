/// @file random_desc.h
/// @brief Descriptions of sources of randomness, such as `xorshift:123`.
///
/// @ref passgen_random_open parses a description and opens the source which
/// it describes. These do the two separately, so that a description can be
/// checked without opening anything, and so that the fuzzing harness can open
/// what was parsed with cheaper Argon2 parameters.
#pragma once
#include <passgen/config.h>
#include <passgen/random.h>
#include <stdbool.h>
#include <stdint.h>

/// Longest master passphrase, domain or token, in bytes.
#define PASSGEN_RANDOM_DESC_STRING_MAX 128

/// Which kind of source a description is for.
typedef enum {
    /// `zero`
    PASSGEN_RANDOM_DESC_ZERO,
    /// `system`
    PASSGEN_RANDOM_DESC_SYSTEM,
    /// `file:<path>`
    PASSGEN_RANDOM_DESC_FILE,
    /// `xorshift:<seed>`
    PASSGEN_RANDOM_DESC_XORSHIFT,
    /// `chacha20:<key>[:<iv>]`
    PASSGEN_RANDOM_DESC_CHACHA20,
    /// `chacha20-argon2:<master-pass>[:<domain>[:<token>]]`
    PASSGEN_RANDOM_DESC_CHACHA20_ARGON2,
} passgen_random_desc_kind;

/// A parsed description. This may hold a master passphrase or a key, so it
/// has to be wiped (with `PASSGEN_CLEAR`) once it is not needed anymore.
typedef struct {
    /// Which kind of source this is, which says which case of @ref data is
    /// used. The zero and system sources do not have any.
    passgen_random_desc_kind kind;
    /// What the description says, depending on the kind.
    union {
        /// For #PASSGEN_RANDOM_DESC_FILE.
        struct {
            /// The path, which points into the description.
            const char *path;
        } file;
        /// For #PASSGEN_RANDOM_DESC_XORSHIFT.
        struct {
            /// The seed, which is not zero.
            uint64_t seed;
        } xorshift;
#ifdef PASSGEN_MONOCYPHER
        /// For #PASSGEN_RANDOM_DESC_CHACHA20.
        struct {
            /// The key, padded with zeroes.
            uint8_t key[32];
            /// The IV, padded with zeroes.
            uint8_t iv[8];
        } chacha20;
        /// For #PASSGEN_RANDOM_DESC_CHACHA20_ARGON2.
        struct {
            /// The master passphrase, NUL-terminated.
            char passphrase[PASSGEN_RANDOM_DESC_STRING_MAX + 1];
            /// The domain, NUL-terminated, empty if there is none.
            char domain[PASSGEN_RANDOM_DESC_STRING_MAX + 1];
            /// The token, NUL-terminated, empty if there is none.
            char token[PASSGEN_RANDOM_DESC_STRING_MAX + 1];
            /// The parameters, NULL for the default ones. Parsing sets this
            /// to NULL, as descriptions cannot have parameters.
            const passgen_argon2_params *params;
        } chacha20_argon2;
#endif
    } data;
} passgen_random_desc;

/// Parses a description of a source of randomness.
///
/// @param desc Set to what the description says, if it is valid
/// @param string The description, as a NUL-terminated string
/// @returns Whether the description is valid. Descriptions of ChaCha20
///   sources are only valid when the library is built with Monocypher.
bool passgen_random_desc_parse(passgen_random_desc *desc, const char *string);

/// Opens the source of randomness which a parsed description is for.
///
/// @param random Where to open it, or NULL to allocate it
/// @param desc The parsed description
/// @returns The source, or NULL if it cannot be opened
passgen_random *passgen_random_desc_open(
    passgen_random *random,
    const passgen_random_desc *desc);
