/// @file array.h
///
/// Defines an array with cheap insertion. Array elements may not be larger
/// than 1024 bytes. All elements must have the same size.

#pragma once
#include <passgen/config.h>
#include <stddef.h>

// Generic array data type.
struct passgen_array {
    void **data;
    size_t capacity;
    size_t items;
};

typedef struct passgen_array passgen_array_t;

// Initialize array. Optionally takes a capacity, if so, memory that is
// sufficiently large to hold `capacity` items will be allocated.
void passgen_array_init(
    passgen_array_t *array,
    size_t capacity);

// Push an item to the back of the array.
void passgen_array_push(
    passgen_array_t *array,
    void *pointer);

// Retrieve an item from the array.
void *passgen_array_get(
    passgen_array_t *array,
    size_t pos);

// Frees an array.
void passgen_array_free(
    passgen_array_t *array);

// Pops the last item off the array and returns it.
void *passgen_array_pop(
    passgen_array_t *array);

void passgen_array_realloc(
    passgen_array_t *array,
    size_t capacity);
