/// @file array.h
///
/// Defines an array with cheap insertion. Array elements may not be larger
/// than 1024 bytes. All elements must have the same size.

#pragma once
#include "passgen/debug.h"
#include <stddef.h>

// forward declaration, to remove dependency.
struct passgen_mem;
typedef struct passgen_mem passgen_mem_t;

struct passgen_array {
  void **data;
  size_t len;
  size_t bins;
#ifdef PASSGEN_DEBUG
  passgen_mem_t *mem;
  size_t size;
#endif
};

typedef struct passgen_array passgen_array_t;

void passgen_array_init(
    passgen_array_t *array, size_t size, passgen_mem_t *mem);
void *
passgen_array_push(passgen_array_t *array, size_t size, passgen_mem_t *mem);
void *passgen_array_get(passgen_array_t *array, size_t size, size_t pos);
void passgen_array_free(
    passgen_array_t *array, size_t size, passgen_mem_t *mem);
void passgen_array_pop(passgen_array_t *array, size_t size, passgen_mem_t *mem);

/* enable this for more optimised lookups. */
#if 0
#  define passgen_array_get(array, size, pos) \
    ((array)->data[pos / (1024 / size)] + (pos % (1024 / size)) * size)
#endif
