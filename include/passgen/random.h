/// @file random.h
/// @author Patrick M. Elsen <pelsen@xfbs.net>
/// @brief Sources of randomness.
///
/// Everything which is generated is chosen using a source of randomness, a
/// @ref passgen_random. Most of the time, this is the one of the system (see
/// @ref passgen_random_system_open). The others give out the same data every
/// time, which is what master passphrase mode is built on (see
/// @ref passgen_random_chacha20_argon2_open), and what the tests use.
#pragma once
#include <passgen/config.h>
#include <passgen/export.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Size of the buffer of a source of randomness, in bytes.
///
/// The source is read from in blocks of this size. Reading more than this at
/// once reads from the source directly. A file has to have at least this many
/// bytes in it to be used as a source.
#define PASSGEN_RANDOM_BUFFER_LENGTH 1024

/// Function which reads data from a source of randomness.
///
/// @param context The context of the source
/// @param dest Where to write the data
/// @param size How many bytes to write
/// @returns How many bytes were written. Anything other than `size` means
///   that the source has failed, and it is not asked again.
typedef size_t passgen_random_read_func(void *context, void *dest, size_t size);

/// Function which closes a source of randomness, releasing its context.
///
/// @param context The context of the source
typedef void passgen_random_close_func(void *context);

/// A source of randomness.
///
/// Sources are opened with one of the `passgen_random_*_open` functions, or
/// with @ref passgen_random_open, which takes a description such as
/// `xorshift:123`. If unsure, use @ref passgen_random_system_open.
///
/// All of these take where to open the source, or NULL:
///
/// - Given a source, such as one on the stack, they open it in place. It has
///   to be closed with @ref passgen_random_close.
/// - Given NULL, they allocate it. It has to be closed and freed with
///   @ref passgen_random_free.
///
/// Either way, they return NULL if it cannot be opened, and there is nothing
/// to close then.
///
/// ```c
/// passgen_random random;
/// if(!passgen_random_open(&random, "system")) {
///     // cannot be opened
/// }
///
/// uint32_t dice = 1 + passgen_random_u32_max(&random, 6);
///
/// passgen_random_close(&random);
/// ```
///
/// A source is not safe to use from more than one thread at a time.
///
/// @par Writing a source
///
/// A source can read from anything, by setting @ref context, @ref read and
/// @ref close, setting @ref failed to false, and then calling
/// @ref passgen_random_reload to fill the buffer. @ref passgen_random_close
/// calls @ref close with the context.
typedef struct {
    /// Data which was read from the source, and not used yet.
    uint8_t buffer[PASSGEN_RANDOM_BUFFER_LENGTH];

    /// How much of @ref buffer is used.
    size_t pos;

    /// The state of the source, which is passed to @ref read and @ref close.
    void *context;

    /// Reads data from the source.
    passgen_random_read_func *read;

    /// Closes the source.
    passgen_random_close_func *close;

    /// Set once the source did not have as much data as was asked for. See
    /// ::passgen_random_failed.
    bool failed;
} passgen_random;

/// Opens the source of randomness which a description says, as the
/// `--random` option of the command-line tool takes it.
///
/// | Source | Description | Example | Opened with |
/// | --- | --- | --- | --- |
/// | System | `system` | `system` | @ref passgen_random_system_open |
/// | File | `file:<path>` | `file:/dev/urandom` | @ref passgen_random_path_open |
/// | Xorshift | `xorshift:<seed>` | `xorshift:123` | @ref passgen_random_xorshift_open |
/// | Zero | `zero` | `zero` | @ref passgen_random_zero_open |
/// | ChaCha20 | `chacha20:<key>[:<iv>]` | `chacha20:00010203...1e1f:0001020304050607` | @ref passgen_random_chacha20_open |
/// | Master passphrase | `chacha20-argon2:<passphrase>[:<domain>[:<token>]]` | `chacha20-argon2:passphrase:example.com:1` | @ref passgen_random_chacha20_argon2_open |
///
/// - The seed of xorshift is a number from 1 to 2^64 - 1, in decimal.
/// - The key and IV of ChaCha20 are in hex (in either case): exactly 64 hex
///   digits for the 32 bytes of the key, and exactly 16 for the 8 bytes of the
///   IV. Without an IV, it is `passgen` (followed by a zero byte).
/// - The master passphrase, the domain and the token are at most 128 bytes
///   each, and cannot contain colons. One colon right after
///   `chacha20-argon2:` is skipped. Keys are derived with
///   ::PASSGEN_ARGON2_PARAMS_DEFAULT.
/// - The ChaCha20 sources are only there when the library is built with
///   Monocypher (`PASSGEN_MONOCYPHER` in `passgen/config.h`).
///
/// @param random Where to open the source, or NULL to allocate it. See
///   @ref passgen_random for who closes or frees it.
/// @param desc The description, as a NUL-terminated string, or NULL for the
///   system source
/// @returns The source, or NULL if the description is not valid, or the
///   source cannot be opened
/// @relatesalso passgen_random
PASSGEN_EXPORT passgen_random *passgen_random_open(passgen_random *random, const char *desc);

/// Whether a description of a source of randomness (see
/// @ref passgen_random_open) may hold a secret: the key of `chacha20`, or the
/// master passphrase of `chacha20-argon2`. Such a description must not be
/// shown, such as in an error message saying that it cannot be opened.
///
/// This only looks at the kind of source, so it works for descriptions which
/// are not valid. A kind which is not known may be a mistyped one which holds
/// a secret, so it counts as one.
///
/// @param desc The description, as a NUL-terminated string
/// @returns Whether the description may hold a secret
PASSGEN_EXPORT bool passgen_random_desc_secret(const char *desc);

/// Opens the source of randomness of the operating system. This is what
/// should be used to generate secrets.
///
/// | System | Source |
/// | --- | --- |
/// | Linux | `getrandom()` |
/// | macOS | `arc4random_buf()` |
/// | Other | `/dev/urandom` |
///
/// @param random Where to open the source, or NULL to allocate it. See
///   @ref passgen_random for who closes or frees it.
/// @returns The source, or NULL if it cannot be opened
/// @relatesalso passgen_random
PASSGEN_EXPORT passgen_random *passgen_random_system_open(passgen_random *random);

/// Opens a file as a source of randomness, such as `/dev/urandom`.
///
/// Once the file has been read to its end, the source has failed (see
/// @ref passgen_random_failed).
///
/// @param random Where to open the source, or NULL to allocate it. See
///   @ref passgen_random for who closes or frees it.
/// @param path The path of the file
/// @returns The source, or NULL if the file cannot be opened, or has less
///   than ::PASSGEN_RANDOM_BUFFER_LENGTH bytes in it
/// @relatesalso passgen_random
PASSGEN_EXPORT passgen_random *
passgen_random_path_open(passgen_random *random, const char *path);

/// Uses a file which is open as a source of randomness.
///
/// Once the file has been read to its end, the source has failed (see
/// @ref passgen_random_failed).
///
/// @param random Where to open the source, or NULL to allocate it. See
///   @ref passgen_random for who closes or frees it.
/// @param file The file, which is read from where it is. The source takes
///   it over: it is closed when the source is, and also when opening it
///   fails.
/// @returns The source, or NULL if the file has less than
///   ::PASSGEN_RANDOM_BUFFER_LENGTH bytes in it
/// @relatesalso passgen_random
PASSGEN_EXPORT passgen_random *
passgen_random_file_open(passgen_random *random, FILE *file);

/// Opens a source which gives out the same data for the same seed, using the
/// xorshift algorithm.
///
/// This is quick, and meant for testing. Do not use it to generate secrets:
/// what it gives out can be predicted from a little of it.
///
/// @param random Where to open the source, or NULL to allocate it. See
///   @ref passgen_random for who closes or frees it.
/// @param seed The seed. With zero, it only gives out zeroes.
/// @returns The source, or NULL if there is no memory for it
/// @relatesalso passgen_random
PASSGEN_EXPORT passgen_random *
passgen_random_xorshift_open(passgen_random *random, uint64_t seed);

/// Opens a source which only gives out zeroes, for testing. With it,
/// everything which is generated is the first choice there is.
///
/// @param random Where to open the source, or NULL to allocate it. See
///   @ref passgen_random for who closes or frees it.
/// @returns The source, or NULL if there is no memory for it
/// @relatesalso passgen_random
PASSGEN_EXPORT passgen_random *
passgen_random_zero_open(passgen_random *random);

#ifdef PASSGEN_MONOCYPHER
/// Opens a source which gives out the same data for the same key and IV, which
/// is the stream of the ChaCha20 cipher.
///
/// What it gives out cannot be predicted without the key, so this can be used
/// for secrets, if the key is one. To get the key from a passphrase, use
/// @ref passgen_random_chacha20_argon2_open.
///
/// Only there when the library is built with Monocypher.
///
/// @param random Where to open the source, or NULL to allocate it. See
///   @ref passgen_random for who closes or frees it.
/// @param key The key, 32 bytes
/// @param iv The IV (nonce), 8 bytes. With another IV, the same key gives out
///   other data.
/// @returns The source, or NULL if there is no memory for it
/// @relatesalso passgen_random
PASSGEN_EXPORT passgen_random *passgen_random_chacha20_open(
    passgen_random *random,
    const uint8_t key[32],
    const uint8_t iv[8]);

/// Parameters for deriving a key from a master passphrase with Argon2id.
///
/// These determine how expensive it is to derive the key, and with that, how
/// expensive it is to guess a master passphrase. Changing any of them results
/// in another key.
typedef struct {
    /// How much memory to use, in KiB. Has to be at least eight times the
    /// number of lanes.
    uint32_t memory;
    /// How many passes to make over the memory. Has to be at least one.
    uint32_t passes;
    /// How many lanes to use. Has to be at least one. This is what other
    /// implementations of Argon2 call parallelism, but they are computed one
    /// after another here.
    uint32_t lanes;
} passgen_argon2_params;

/// Version 1 of the Argon2 parameters used to derive a key from a master
/// passphrase: Argon2id with 19 MiB of memory, 2 passes and 1 lane. This is the
/// minimum that OWASP recommends.
///
/// These are never changed, as that would change every passphrase derived
/// from a master passphrase. Stronger parameters get a new version.
PASSGEN_EXPORT extern const passgen_argon2_params PASSGEN_ARGON2_PARAMS_V1;

/// The Argon2 parameters which ::passgen_random_chacha20_argon2_open uses
/// when it is not given any. This is the latest version, so it changes when a
/// new version is added. Use a specific version to always get the same output.
PASSGEN_EXPORT extern const passgen_argon2_params PASSGEN_ARGON2_PARAMS_DEFAULT;

/// Opens a source which gives out the same data for the same master
/// passphrase, domain and token. This is master passphrase mode: everything
/// which is generated with it can be generated again from the passphrase.
///
/// The key is derived using Argon2id, with the fixed salt `passgen/argon2id`,
/// the domain as the secret key and the token as the associated data. It is
/// then used for ChaCha20 with the IV `passgen` (followed by a zero byte), as
/// with @ref passgen_random_chacha20_open. Deriving the key takes as much
/// memory as the parameters say (19 MiB by default), and takes a while, on
/// purpose. The derived key is wiped once it is not needed.
///
/// The domain and the token are optional. They give different passphrases
/// for the same master passphrase: the domain for different websites, the
/// token for different accounts on the same one, or as a counter to change a
/// passphrase.
///
/// What this gives out for the same inputs and parameters never changes
/// between versions of the library.
///
/// Only there when the library is built with Monocypher.
///
/// @param random Where to open the source, or NULL to allocate it. See
///   @ref passgen_random for who closes or frees it.
/// @param passphrase Master passphrase, as a NUL-terminated string
/// @param domain Domain name, as a NUL-terminated string (may be NULL)
/// @param token Token, used as additional input, as a NUL-terminated string
///   (may be NULL)
/// @param params Argon2 parameters to use, or NULL for
///   ::PASSGEN_ARGON2_PARAMS_DEFAULT
/// @returns The random object, or NULL if the parameters are not valid, or
///   the memory for deriving the key could not be allocated
/// @relatesalso passgen_random
PASSGEN_EXPORT passgen_random *passgen_random_chacha20_argon2_open(
    passgen_random *random,
    const char *passphrase,
    const char *domain,
    const char *token,
    const passgen_argon2_params *params);
#endif

/// Closes a source which was opened in place, and clears it.
///
/// Use @ref passgen_random_free for one which was allocated by opening it with
/// NULL.
///
/// @param random The source
/// @relatesalso passgen_random
PASSGEN_EXPORT void passgen_random_close(passgen_random *random);

/// Closes and frees a source which was allocated by opening it with NULL.
///
/// Use @ref passgen_random_close for one which was opened in place.
///
/// @param random The source
/// @relatesalso passgen_random
PASSGEN_EXPORT void passgen_random_free(passgen_random *random);

/// Reads data from a source.
///
/// If the source does not have as much data as is asked for, this writes
/// zeroes, and the source has failed: see @ref passgen_random_failed.
///
/// @param random The source
/// @param dest Where to write the data
/// @param bytes How many bytes to write
/// @relatesalso passgen_random
PASSGEN_EXPORT void passgen_random_read(passgen_random *random, void *dest, size_t bytes);

/// Reads a `uint8_t` from a source.
///
/// @param random The source
/// @relatesalso passgen_random
PASSGEN_EXPORT uint8_t passgen_random_u8(passgen_random *random);

/// Reads a `uint16_t` from a source.
///
/// @param random The source
/// @relatesalso passgen_random
PASSGEN_EXPORT uint16_t passgen_random_u16(passgen_random *random);

/// Reads a `uint32_t` from a source.
///
/// @param random The source
/// @relatesalso passgen_random
PASSGEN_EXPORT uint32_t passgen_random_u32(passgen_random *random);

/// Reads a `uint64_t` from a source.
///
/// @param random The source
/// @relatesalso passgen_random
PASSGEN_EXPORT uint64_t passgen_random_u64(passgen_random *random);

/// Reads a `bool` from a source, which is true or false equally often.
///
/// @param random The source
/// @relatesalso passgen_random
PASSGEN_EXPORT bool passgen_random_bool(passgen_random *random);

/// Reads a `uint8_t` which is smaller than @p max from a source. Every number
/// from zero up to @p max - 1 is equally likely.
///
/// @param random The source
/// @param max Must not be zero, as nothing is smaller than that. The library
///   aborts if it is.
/// @relatesalso passgen_random
PASSGEN_EXPORT uint8_t passgen_random_u8_max(passgen_random *random, uint8_t max);

/// Reads a `uint16_t` which is smaller than @p max from a source. Every number
/// from zero up to @p max - 1 is equally likely.
///
/// @param random The source
/// @param max Must not be zero, as nothing is smaller than that. The library
///   aborts if it is.
/// @relatesalso passgen_random
PASSGEN_EXPORT uint16_t passgen_random_u16_max(passgen_random *random, uint16_t max);

/// Reads a `uint32_t` which is smaller than @p max from a source. Every number
/// from zero up to @p max - 1 is equally likely.
///
/// @param random The source
/// @param max Must not be zero, as nothing is smaller than that. The library
///   aborts if it is.
/// @relatesalso passgen_random
PASSGEN_EXPORT uint32_t passgen_random_u32_max(passgen_random *random, uint32_t max);

/// Reads a `uint64_t` which is smaller than @p max from a source. Every number
/// from zero up to @p max - 1 is equally likely.
///
/// @param random The source
/// @param max Must not be zero, as nothing is smaller than that. The library
///   aborts if it is.
/// @relatesalso passgen_random
PASSGEN_EXPORT uint64_t passgen_random_u64_max(passgen_random *random, uint64_t max);

/// Fills the buffer of a source with new data from it, throwing away what was
/// not used yet. This is needed once when writing a source (see
/// @ref passgen_random).
///
/// @param random The source
/// @relatesalso passgen_random
PASSGEN_EXPORT void passgen_random_reload(passgen_random *random);

/// Whether the source has failed, which is when it did not have as much data
/// as was asked for (a file which has been read to the end, for example).
///
/// A source which has failed stays that way, and gives out nothing but zeroes.
/// Anything which was made using it has to be thrown away. @ref
/// passgen_generate checks this, and fails with @ref PASSGEN_ERROR_RANDOM. If
/// you read from a source yourself, you have to check it once you are done.
///
/// @param random The source
/// @relatesalso passgen_random
PASSGEN_EXPORT bool passgen_random_failed(const passgen_random *random);

#ifdef PASSGEN_MONOCYPHER
/// Seeks to a position in the ChaCha20 stream, so that what is read next is
/// what was read at that position. Seeking to zero starts over.
///
/// This only works for sources opened with ::passgen_random_chacha20_open or
/// ::passgen_random_chacha20_argon2_open. Anything else is left as it is.
///
/// @relatesalso passgen_random
/// @param random The source
/// @param position The position, in bytes from the start of the stream
/// @returns Whether it seeked, which is false if the source is not ChaCha20
PASSGEN_EXPORT bool
passgen_random_chacha20_seek(passgen_random *random, size_t position);
#endif

#ifdef __cplusplus
}
#endif
