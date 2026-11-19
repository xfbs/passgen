/// @file master.h
/// @brief Master passphrase mode: deriving passphrases from a master
/// passphrase.
///
/// Everything which is generated in master passphrase mode can be generated
/// again from the master passphrase, the domain and the user, without storing
/// anything. This works in two steps:
///
/// 1. @ref passgen_master_derive derives a master key from the master
///    passphrase (and optionally a seed), which is expensive on purpose, so
///    that guessing master passphrases is expensive as well.
/// 2. @ref passgen_master_random_open opens a source of randomness for a
///    domain and a user, from the master key, which is cheap.
///
/// @ref passgen_master_hash gives a value which can be stored to tell whether
/// a master passphrase was entered correctly, without storing it.
///
/// Version 1 is defined like this, with the master passphrase, domain and
/// user as they are (bytes, not normalized in any way):
///
/// - The master key is Argon2id (version 0x13) of the master passphrase, with
///   the salt `passgen/master/1`, the seed (if any) as the secret key, no
///   associated data, one lane, and the memory and passes of the level (see
///   @ref passgen_master_level), 32 bytes long.
/// - The hash is BLAKE2b, 32 bytes long, keyed with the master key, of
///   `passgen/hash`.
/// - The key for a domain and a user is BLAKE2b, 32 bytes long, keyed with
///   the master key, of `passgen/site`, followed by the length of the domain
///   in bytes (64 bits, little-endian), the domain, the length of the user in
///   the same way, and the user.
/// - The source of randomness is ChaCha20 (the original variant, with a 64
///   bit counter) with this key and the IV `passgen` followed by a zero byte,
///   as with @ref passgen_random_chacha20_open.
///
/// What a version gives out for the same inputs never changes. Changes to how
/// this works get a new version.
///
/// Only there when the library is built with Monocypher.
#pragma once
#include <passgen/config.h>
#include <passgen/error.h>
#include <passgen/export.h>
#include <passgen/random.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef PASSGEN_MONOCYPHER

/// The latest version of how passphrases are derived.
#define PASSGEN_MASTER_VERSION 1

/// The level which is used when none is chosen.
#define PASSGEN_MASTER_LEVEL_DEFAULT 2

/// The highest level.
#define PASSGEN_MASTER_LEVEL_MAX 3

/// Length of a master key, in bytes.
#define PASSGEN_MASTER_KEY_SIZE 32

/// Length of the hash of a master key, in bytes.
#define PASSGEN_MASTER_HASH_SIZE 32

/// Longest seed, in bytes.
#define PASSGEN_MASTER_SEED_MAX 64

/// How much effort deriving a master key takes, for a level.
typedef struct {
    /// How much memory Argon2id uses, in KiB.
    uint32_t memory;
    /// How many passes Argon2id makes over the memory.
    uint32_t passes;
} passgen_master_cost;

/// A master key, derived from a master passphrase.
///
/// This is a secret: anything which can be derived from the master passphrase
/// can be derived from it. Wipe it with @ref passgen_master_wipe once it is
/// not needed anymore.
typedef struct {
    /// The version of how it was derived, which says how passphrases are
    /// derived from it.
    unsigned version;
    /// The key.
    uint8_t key[PASSGEN_MASTER_KEY_SIZE];
} passgen_master;

/// The cost of deriving a master key, for a version and a level. These never
/// change for a version.
///
/// | Level | Memory | Passes |
/// | --- | --- | --- |
/// | 1 | 64 MiB | 3 |
/// | 2 | 256 MiB | 3 |
/// | 3 | 1 GiB | 3 |
///
/// @param version Version, which is 1
/// @param level Level, from 1 to @ref PASSGEN_MASTER_LEVEL_MAX
/// @param cost Set to the cost
/// @returns Whether there is such a version and level
PASSGEN_EXPORT bool passgen_master_level(
    unsigned version,
    unsigned level,
    passgen_master_cost *cost);

/// Derives a master key from a master passphrase. This takes as much memory
/// as the level says, and takes a while, on purpose.
///
/// @param master Set to the master key
/// @param version Version, which is 1
/// @param level Level, from 1 to @ref PASSGEN_MASTER_LEVEL_MAX
/// @param passphrase The master passphrase, as a NUL-terminated string
/// @param seed Seed, which another master key is derived with, or NULL
/// @param seed_size Length of the seed in bytes, at most
///   @ref PASSGEN_MASTER_SEED_MAX. Zero is the same as no seed.
/// @returns @ref PASSGEN_OK, @ref PASSGEN_ERROR_INVALID_ARGUMENT if there is
///   no such version or level or the seed is too long, or
///   @ref PASSGEN_ERROR_OUT_OF_MEMORY
/// @relatesalso passgen_master
PASSGEN_EXPORT passgen_status passgen_master_derive(
    passgen_master *master,
    unsigned version,
    unsigned level,
    const char *passphrase,
    const uint8_t *seed,
    size_t seed_size);

/// Derives a master key with a given cost, rather than one of a level. This
/// is what @ref passgen_master_derive does, and is there so that tests and
/// fuzzing can use small costs.
///
/// @returns @ref PASSGEN_OK, @ref PASSGEN_ERROR_INVALID_ARGUMENT if there is
///   no such version, the seed is too long, or the cost is not valid (less
///   than 8 KiB of memory or no passes), or @ref PASSGEN_ERROR_OUT_OF_MEMORY
/// @relatesalso passgen_master
PASSGEN_EXPORT passgen_status passgen_master_derive_cost(
    passgen_master *master,
    unsigned version,
    const passgen_master_cost *cost,
    const char *passphrase,
    const uint8_t *seed,
    size_t seed_size);

/// The hash of a master key, which says whether a master passphrase was
/// entered correctly, without saying what it is. A prefix of it is enough to
/// tell, and says less about the master passphrase.
///
/// @param master The master key
/// @param hash Set to the hash
/// @relatesalso passgen_master
PASSGEN_EXPORT void passgen_master_hash(
    const passgen_master *master,
    uint8_t hash[PASSGEN_MASTER_HASH_SIZE]);

/// Opens a source of randomness for a domain and a user, which gives out the
/// same data for the same master key, domain and user.
///
/// @param random Where to open the source, or NULL to allocate it. See
///   @ref passgen_random for who closes or frees it.
/// @param master The master key
/// @param domain The domain, such as the website, as a NUL-terminated string
///   (NULL is the same as empty)
/// @param user The user, such as the login on the website, as a
///   NUL-terminated string (NULL is the same as empty)
/// @returns The source, or NULL if there is no memory for it
/// @relatesalso passgen_master
PASSGEN_EXPORT passgen_random *passgen_master_random_open(
    passgen_random *random,
    const passgen_master *master,
    const char *domain,
    const char *user);

/// Wipes a master key.
///
/// @param master The master key
/// @relatesalso passgen_master
PASSGEN_EXPORT void passgen_master_wipe(passgen_master *master);

#endif

#ifdef __cplusplus
}
#endif
