/// @file random.h
///
/// Functions used to extract random data. Provides an interface around a
/// random data device, typically `/dev/urandom`, that can be used safely.

#pragma once
#include <stdint.h>
#include <stdio.h>

/// Size of the random data ring buffer.
/// Requests for more than this result in a direct call to read(),
/// requests smaller than this are read from the buffer.
#define RANDOM_H_BUFEN 1024

typedef size_t passgen_random_read_func(void *dest, size_t size, void *data);
typedef void passgen_random_close_func(void *data);

/// Random object. Caches some random data to reduce read() syscalls.
typedef struct random_t {
  /// Ring buffer to hold random data in.
  uint8_t buffer[RANDOM_H_BUFEN];

  /// Current position in the ring buffer.
  size_t pos;

  /// Device to read random data from.
  void *data;

  passgen_random_read_func *read;
  passgen_random_close_func *close;
} random_t;

/// Allocates and opens new random object. Returns `NULL` on error. Uses
/// `/dev/urandom` as random device.
///
/// @par Example
///
/// ```c
/// random_t *random = random_new();
///
/// assert(random != NULL);
///
/// // use random
///
/// random_free(random);
/// ```
random_t *random_new();

/// Allocates and opens a new random object using @p path as random device.
/// Not recommended to use `/dev/random` as randomness device, use the default
/// `/dev/urandom` instead.
random_t *random_new_path(const char *path);

/// Allocates and opens a new random object with @p file as randomness source.
random_t *random_new_file(FILE *file);

/// Opens a new, existing random object. Returns `NULL` on failure. Uses
/// `/dev/urandom` as random device.
///
/// @par Example
///
/// ```c
/// random_t random;
///
/// assert(random_open(&random) != NULL);
///
/// // use random
///
/// random_close(&random);
/// ```
random_t *random_open(random_t *random);

/// Opens a new, existing random object using @p path as random device.
random_t *random_open_path(random_t *random, const char *path);

/// Opens a new random object with @p file as randomness source.
random_t *random_open_file(random_t *random, FILE *file);

/// Close @p random. Use this with object opened by random_open().
void random_close(random_t *random);

/// Close and free @p random. Use this with objects allocated by random_new().
void random_free(random_t *random);

/// Read random data from @a random.
///
/// @param dest destination to write to, must be large enough and not NULL.
/// @param bytes how many bytes to write.
void random_read(random_t *random, void *dest, size_t bytes);

/// Read a `uint8_t` from @p random.
uint8_t random_uint8(random_t *random);

/// Read a `uint16_t` from @p random.
uint16_t random_uint16(random_t *random);

/// Read a `uint32_t` from @p random.
uint32_t random_uint32(random_t *random);

/// Read a `uint64_t` from @p random.
uint64_t random_uint64(random_t *random);

/// Read a `uint8_t` that is strictly smaller than @p max from @p random.
///
/// @param max must be nonzero.
uint8_t random_uint8_max(random_t *random, uint8_t max);

/// Read a `uint16_t` that is strictly smaller than @p max from @p random.
///
/// @param max must be nonzero.
uint16_t random_uint16_max(random_t *random, uint16_t max);

/// Read a `uint32_t` that is strictly smaller than @p max from @p random.
///
/// @param max must be nonzero.
uint32_t random_uint32_max(random_t *random, uint32_t max);

/// Read a `uint64_t` that is strictly smaller than @p max from @p random.
///
/// @param max must be nonzero.
uint64_t random_uint64_max(random_t *random, uint64_t max);

size_t passgen_random_read_file(void *dest, size_t size, void *data);
size_t passgen_random_read_system(void *dest, size_t size, void *data);

void passgen_random_close_file(void *data);
void passgen_random_close_system(void *data);
