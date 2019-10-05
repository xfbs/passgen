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

passgen_array_t passgen_array_init(size_t size, passgen_mem_t *mem);
void *passgen_array_push(passgen_array_t *array, size_t size, passgen_mem_t *mem);
void *passgen_array_get(passgen_array_t *array, size_t size, size_t pos);
void passgen_array_free(passgen_array_t *array, size_t size, passgen_mem_t *mem);
void passgen_array_pop(passgen_array_t *array, size_t size, passgen_mem_t *mem);

#ifndef PASSGEN_DEBUG
#define passgen_array_get(array, size, pos) \
    ((array)->data[pos / (4096 / size)] + (pos % (4096 / size)) * size)
#endif
