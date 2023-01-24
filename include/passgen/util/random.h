/// @file random.h
/// @author Patrick M. Elsen <pelsen@xfbs.net>
/// @brief Generate random data from various sources.
///
/// Functions used to extract random data. Provides an interface around a
/// random data device, typically `/dev/urandom`, that can be used safely.
#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

/// Size of the random data ring buffer.
/// Requests for more than this result in a direct call to read(),
/// requests smaller than this are read from the buffer.
#define RANDOM_H_BUFEN 1024

typedef size_t passgen_random_read_func(void *dest, size_t size, void *data);
typedef void passgen_random_close_func(void *data);

/// Random object. Caches some random data to reduce read() syscalls.
typedef struct passgen_random {
    /// Ring buffer to hold random data in.
    uint8_t buffer[RANDOM_H_BUFEN];

    /// Current position in the ring buffer.
    size_t pos;

    /// Device to read random data from.
    void *data;

    passgen_random_read_func *read;
    passgen_random_close_func *close;
} passgen_random;

/// Allocates and opens new random object. Returns `NULL` on error. Uses
/// `/dev/urandom` as random device.
///
/// @par Example
///
/// ```c
/// passgen_random *random = passgen_random_new();
///
/// assert(random != NULL);
///
/// // use random
///
/// passgen_random_free(random);
/// ```
passgen_random *passgen_random_new(const char *desc);

/// Allocates and opens a new random object using @p path as random device.
/// Not recommended to use `/dev/random` as randomness device, use the default
/// `/dev/urandom` instead.
passgen_random *passgen_random_new_path(const char *path);

/// Allocates and opens a new random object with @p file as randomness source.
passgen_random *passgen_random_new_file(FILE *file);

/// Allocates and opens a new random object using the xorshift PRNG.
passgen_random *passgen_random_new_xorshift(uint64_t seed);

/// Allocates and opens a new random object using the zero randomness generator.
passgen_random *passgen_random_new_zero();

/// Opens a new, existing random object. Returns `NULL` on failure. Uses
/// `/dev/urandom` as random device.
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
passgen_random *passgen_random_open(passgen_random *random, const char *desc);

/// Opens a new, existing random object using @p path as random device.
passgen_random *
passgen_random_open_path(passgen_random *random, const char *path);

/// Opens a new random object with @p file as randomness source.
passgen_random *
passgen_random_open_file(passgen_random *random, FILE *file);

/// Opens a new random object using the xorshift PRNG.
passgen_random *
passgen_random_open_xorshift(passgen_random *random, uint64_t seed);

/// Opens a new random object using the zero randomness generator
passgen_random *
passgen_random_open_zero(passgen_random *random);

/// Close @p random. Use this with object opened by passgen_random_open().
void passgen_random_close(passgen_random *random);

/// Close and free @p random. Use this with objects allocated by
/// passgen_random_new().
void passgen_random_free(passgen_random *random);

/// Read random data from @a random.
///
/// @param dest destination to write to, must be large enough and not NULL.
/// @param bytes how many bytes to write.
void passgen_random_read(passgen_random *random, void *dest, size_t bytes);

/// Read a `uint8_t` from @p random.
uint8_t passgen_random_u8(passgen_random *random);

/// Read a `uint16_t` from @p random.
uint16_t passgen_random_u16(passgen_random *random);

/// Read a `uint32_t` from @p random.
uint32_t passgen_random_u32(passgen_random *random);

/// Read a `uint64_t` from @p random.
uint64_t passgen_random_u64(passgen_random *random);

/// Generate a random boolean.
bool passgen_random_bool(passgen_random *random);

/// Read a `uint8_t` that is strictly smaller than @p max from @p random.
///
/// @param max must be nonzero.
uint8_t passgen_random_u8_max(passgen_random *random, uint8_t max);

/// Read a `uint16_t` that is strictly smaller than @p max from @p random.
///
/// @param max must be nonzero.
uint16_t passgen_random_u16_max(passgen_random *random, uint16_t max);

/// Read a `uint32_t` that is strictly smaller than @p max from @p random.
///
/// @param max must be nonzero.
uint32_t passgen_random_u32_max(passgen_random *random, uint32_t max);

/// Read a `uint64_t` that is strictly smaller than @p max from @p random.
///
/// @param max must be nonzero.
uint64_t passgen_random_u64_max(passgen_random *random, uint64_t max);

size_t passgen_random_read_file(void *dest, size_t size, void *data);
size_t passgen_random_read_system(void *dest, size_t size, void *data);
size_t passgen_random_read_xorshift(void *dest, size_t size, void *data);

void passgen_random_close_file(void *data);
void passgen_random_close_system(void *data);
void passgen_random_close_xorshift(void *data);
