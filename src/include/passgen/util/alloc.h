/// @file alloc.h
/// @brief Allocating memory, for use inside of the library.
///
/// Everything which the library allocates goes through these, so that the
/// allocator can be replaced (see @ref passgen_allocator_set).
#pragma once
#include <passgen/allocator.h>

/// Allocates memory.
///
/// @param size How many bytes to allocate. Zero is allowed, and allocates one
///   byte.
/// @returns The memory, or NULL if there is none
void *passgen_alloc(size_t size);

/// Allocates memory which is zeroed.
///
/// @param size How many bytes to allocate
/// @returns The memory, or NULL if there is none
void *passgen_alloc_zeroed(size_t size);

/// Changes the size of an allocation, which may move it.
///
/// @param ptr The allocation. If it is NULL, this allocates.
/// @param old_size The size which was asked for when allocating it
/// @param new_size The size which it should have
/// @returns The memory, or NULL if there is none. In that case, the
///   allocation is still there.
void *passgen_resize(void *ptr, size_t old_size, size_t new_size);

/// Releases an allocation.
///
/// @param ptr The allocation. If it is NULL, nothing happens.
/// @param size The size which was asked for when allocating it
void passgen_free(void *ptr, size_t size);

/// Copies a string into newly allocated memory. It is released using
/// @ref passgen_free_string.
///
/// @param string NUL-terminated string
/// @returns The copy, or NULL if there is no memory for it
char *passgen_alloc_string(const char *string);

/// Releases a string which was allocated by @ref passgen_alloc_string.
///
/// @param string The string. If it is NULL, nothing happens.
void passgen_free_string(char *string);
