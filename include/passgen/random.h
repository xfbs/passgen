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
#include <passgen/crypto.h>

/// Size of the random data ring buffer.
///
/// Requests for more than this result in a direct call to the @ref
/// passgen_random_read_fun, requests smaller than this are read from the
/// buffer.
#define PASSGEN_RANDOM_BUFFER_LENGTH 1024

/// Type of function used to read more random data.
typedef size_t passgen_random_read_func(void *context, void *dest, size_t size);

/// Type of function used to close the randomness source.
typedef void passgen_random_close_func(void *context);

/// Randomness source.
typedef struct {
    /// Ring buffer to hold random data in.
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
/// @par Example
///
/// ```c
/// passgen_random random;
///
/// assert(passgen_random_open(&random) != NULL);
///
/// // use random
///
/// passgen_random_close(&random);
/// ```
///
/// @memberof passgen_random
passgen_random *passgen_random_open(passgen_random *random, const char *desc);

/// Opens the default system randomness source.
///
/// @memberof passgen_random
passgen_random *passgen_random_system_open(passgen_random *random);

/// Opens a new, existing random object using @p path as random device.
///
/// @memberof passgen_random
passgen_random *
passgen_random_open_path(passgen_random *random, const char *path);

/// Opens a new random object with @p file as randomness source.
///
/// @memberof passgen_random
passgen_random *
passgen_random_open_file(passgen_random *random, FILE *file);

/// Opens a new random object using the xorshift PRNG.
///
/// When using this, ensure that the seed is not set to zero. Otherwise all
/// data generated will be zeroes.
///
/// @memberof passgen_random
passgen_random *
passgen_random_xorshift_open(passgen_random *random, uint64_t seed);

/// Opens a new random object using the zero randomness generator
///
/// @memberof passgen_random
passgen_random *
passgen_random_zero_open(passgen_random *random);

/// Opens a new random object using pseudorandom data generated using the
/// ChaCha20 stream cipher.
///
/// @memberof passgen_random
passgen_random *passgen_random_chacha20_open(
    passgen_random *random,
    const uint8_t key[32],
    const uint8_t iv[8]);

/// Opens a new random object using pseudorandom data generated by the ChaCha20
/// stream cipher, with a key derived from the supplied passphrase, domain and
/// token.
///
/// @memberof passgen_random
passgen_random *passgen_random_chacha20_argon2_open(
    passgen_random *random,
    const uint8_t *passphrase,
    const uint8_t *domain,
    const uint8_t *token,
    passgen_argon2_config *config);

/// Close @p random. Use this with object opened by passgen_random_open().
///
/// @memberof passgen_random
void passgen_random_close(passgen_random *random);

/// Close and free @p random. Use this with objects allocated by
/// passgen_random_new().
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
