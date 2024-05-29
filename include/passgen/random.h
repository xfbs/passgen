/// @file random.h
/// @author Patrick M. Elsen <pelsen@xfbs.net>
/// @brief Generate random data from various sources.
///
/// Functions used to extract random data. Provides a buffered interface around
/// a random data device that can be used safely. Can be used to access data
/// from the system randomness generator, or a pseudorandom generator for
/// deterministic randomness.
#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <passgen/config.h>

#ifdef PASSGEN_MONOCYPHER
#include <passgen/monocypher.h>
#endif

/// Size of the random data ring buffer.
///
/// Requests for more than this result in a direct call to the
/// ::passgen_random_read_func, requests smaller than this are read from the
/// buffer.
#define PASSGEN_RANDOM_BUFFER_LENGTH 1024

/// Function used to read more random data.
///
/// @param context Context pointer of the randomness source.
/// @param dest Destination to write data to.
/// @param size Amount of bytes of data to write.
typedef size_t passgen_random_read_func(void *context, void *dest, size_t size);

/// Function used to close the randomness source.
///
/// @param context Context pointer of the randomness source.
typedef void passgen_random_close_func(void *context);

/// Source of randomness, backed by the system randomness generator or a custom strategy.
///
/// Provides methods for generating random values from this source. This module
/// implements several sources of randomness, if you are unsure which to use,
/// @p passgen_random_system_open is a good default.
///
/// Randomness sources are like file handles: they need to be opened, for
/// example with ::passgen_random_open. When you are done using them, they
/// need to be closed, for example with ::passgen_random_close. If you forget
/// to close them, you might leak memory or randomness state.
///
/// @par Example
///
/// ```c
/// passgen_random random;
///
/// // open
/// passgen_random_open(&random, "system");
///
/// // use it
/// uint8_t value_u8 = passgen_random_u8(&random);
/// uint16_t value_u16 = passgen_random_u16(&random);
/// uint32_t value_u32 = passgen_random_u32_max(&random, 142413413);
///
/// // close it
/// passgen_random_close(&random);
/// ```
typedef struct {
    /// Buffer to hold random data in.
    uint8_t buffer[PASSGEN_RANDOM_BUFFER_LENGTH];

    /// Current position in the ring buffer.
    size_t pos;

    /// Context for randomness source. What this contains is implementation-defined.
    void *context;

    /// Function used to read more random data.
    passgen_random_read_func *read;

    /// Function used to close randomness source.
    passgen_random_close_func *close;
} passgen_random;

/// Opens a new random object. If given a NULL pointer, allocates memory for it.
/// Returns `NULL` on failure.
///
/// It takes a string description as to determine what kind of randomness source to create. When this is NULL,
/// it defaults to creating a system randomness source. Other strategies are also available:
///
/// | Strategy | Syntax | Example |
/// | --- | --- | --- |
/// | System | `system` | `system` |
/// | Xorshift | `xorshift:<seed>` | `xorshift:123` |
/// | Zero | `zero` | `zero` |
/// | File | `file:<path>` | `file:/dev/zero` |
/// | ChaCha20 | `chacha20:<key>:<iv>` | `chacha20:aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa:iviviviv` |
/// | ChaCha20-Argon2 | `chacha20-argon2:<master-pass>[:<domain>[:<token>]]` | `chacha20-argon2:mymasterpass:example.com:1` |
///
/// @par Example
///
/// ```c
/// passgen_random random;
///
/// // open new random device
/// assert(passgen_random_open(&random, "system") != NULL);
///
/// // generate random values
/// uint8_t value = passgen_random_u8(&random);
///
/// // close random device
/// passgen_random_close(&random);
/// ```
///
/// @memberof passgen_random
passgen_random *passgen_random_open(passgen_random *random, const char *desc);

/// Opens the default system randomness source.
///
/// @par Source of Randomness
///
/// The source of randomness depends on which platform you are using this.
///
/// | Platform | Source |
/// | --- | --- |
/// | Linux | `getrandom()` syscall |
/// | macOS | `arc4random_buf()` syscall |
/// | UNIX | `/dev/urandom` |
///
/// @par Example
///
/// ```c
/// passgen_random random;
/// passgen_random_system_open(&random);
/// ```
///
/// @param desc String describing which source of randomness to create.
/// @memberof passgen_random
passgen_random *passgen_random_system_open(passgen_random *random);

/// Opens a new, existing random object using @p path as random device.
///
/// @param path Path of file to use as randomness source.
/// @memberof passgen_random
passgen_random *
passgen_random_path_open(passgen_random *random, const char *path);

/// Opens a new random object with @p file as randomness source.
///
/// @param file File object to use as randomness source.
/// @memberof passgen_random
passgen_random *
passgen_random_file_open(passgen_random *random, FILE *file);

/// Opens a new random object using the xorshift PRNG.
///
/// When using this, ensure that the seed is not set to zero. Otherwise all
/// data generated will be zeroes.
///
/// Note that this you should not use this to generate secrets as it is not
/// very secure. Use ::passgen_random_chacha20_open to generate secure
/// pseudorandom data instead.
///
/// @param seed Seed to use for the algorithm.
/// @memberof passgen_random
passgen_random *
passgen_random_xorshift_open(passgen_random *random, uint64_t seed);

/// Opens a new random object using the zero randomness generator
///
/// Note that this randomness generator always returns zeroes. It only exists
/// for testing purposes.
///
/// @memberof passgen_random
passgen_random *
passgen_random_zero_open(passgen_random *random);

#ifdef PASSGEN_MONOCYPHER
/// Opens a new random object using pseudorandom data generated using the
/// ChaCha20 stream cipher.
///
/// @param key Key to use. Must be a 32-byte value with a lot of entropy. It is recommended that you use a hash function to generate it.
/// @param iv Initialisation vector, used to randomize the output for the same key.
/// @memberof passgen_random
passgen_random *passgen_random_chacha20_open(
    passgen_random *random,
    const uint8_t key[32],
    const uint8_t iv[8]);

/// Opens a new random object using pseudorandom data generated by the ChaCha20
/// stream cipher, with a key derived from the supplied passphrase, domain and
/// token.
///
/// The domain and token parameters are optional, but allow you to generate distinct
/// passphrases using the same master passphrase. You may use the token to generate
/// distinct passphrases for multiple logins to the same domain, for example. Or you
/// may use it as a counter to rotate your passphrases periodically.
///
/// @param passphrase Master passphrase
/// @param domain Domain name
/// @param token Token, used as additional input
/// @memberof passgen_random
passgen_random *passgen_random_chacha20_argon2_open(
    passgen_random *random,
    const uint8_t *passphrase,
    const uint8_t *domain,
    const uint8_t *token,
    passgen_argon2_config *config);
#endif

/// Close @p random.
///
/// Always call this when you are done using a randomness source to avoid a
/// memory leak and clear the memory.
///
/// @memberof passgen_random
void passgen_random_close(passgen_random *random);

/// Close and free @p random.
///
/// Always call this when you are done using a heap-allocated randomness source
/// to avoid a memory leak and clear the memory.
///
/// @memberof passgen_random
void passgen_random_free(passgen_random *random);

/// Read random data from @a random.
///
/// @param dest destination to write to, must be large enough and not NULL.
/// @param bytes how many bytes to write.
///
/// @memberof passgen_random
void passgen_random_read(passgen_random *random, void *dest, size_t bytes);

/// Read a `uint8_t` from @p random.
///
/// @memberof passgen_random
uint8_t passgen_random_u8(passgen_random *random);

/// Read a `uint16_t` from @p random.
///
/// @memberof passgen_random
uint16_t passgen_random_u16(passgen_random *random);

/// Read a `uint32_t` from @p random.
///
/// @memberof passgen_random
uint32_t passgen_random_u32(passgen_random *random);

/// Read a `uint64_t` from @p random.
///
/// @memberof passgen_random
uint64_t passgen_random_u64(passgen_random *random);

/// Generate a random boolean.
///
/// @memberof passgen_random
bool passgen_random_bool(passgen_random *random);

/// Read a `uint8_t` that is strictly smaller than @p max from @p random.
///
/// @param max must be nonzero.
/// @memberof passgen_random
uint8_t passgen_random_u8_max(passgen_random *random, uint8_t max);

/// Read a `uint16_t` that is strictly smaller than @p max from @p random.
///
/// @param max must be nonzero.
/// @memberof passgen_random
uint16_t passgen_random_u16_max(passgen_random *random, uint16_t max);

/// Read a `uint32_t` that is strictly smaller than @p max from @p random.
///
/// @param max must be nonzero.
/// @memberof passgen_random
uint32_t passgen_random_u32_max(passgen_random *random, uint32_t max);

/// Read a `uint64_t` that is strictly smaller than @p max from @p random.
///
/// @param max must be nonzero.
/// @memberof passgen_random
uint64_t passgen_random_u64_max(passgen_random *random, uint64_t max);

/// Reloads the randomness buffer of the randomness source.
///
/// This will discard any unused random data in the buffer.
///
/// @memberof passgen_random
void passgen_random_reload(passgen_random *random);

/// Seeks to a specified position in the ChaCha20 stream.
///
/// Only use this with randomness sources from @p passgen_random_chacha20_open
/// or @p passgen_random_chacha20_argon2_open. If you use it with any other
/// randomness source, it will panic at runtime.
///
/// @memberof passgen_random
void passgen_random_chacha20_seek(passgen_random *random, size_t position);
