/// @file allocator.h
/// @brief Replacing how the library allocates memory.
///
/// All of the memory which the library allocates comes from one allocator,
/// which is shared by everything in the process. Unless it is replaced, this
/// is `malloc()`, `realloc()` and `free()`.
///
/// Replacing it is for running where there is no heap (with an allocator
/// which hands out memory from a buffer), for keeping what is allocated out of
/// swap, and for testing what happens when memory runs out.
#pragma once
#include <passgen/export.h>
#include <stddef.h>

/// An allocator. The sizes of allocations are passed back in when they are
/// resized or freed, so it does not have to keep track of them.
///
/// What is returned has to be aligned for any type, as with `malloc()`.
typedef struct passgen_allocator {
    /// Allocates memory.
    ///
    /// @param state The state of the allocator
    /// @param size How many bytes to allocate, never zero
    /// @returns The memory, or NULL if there is none
    void *(*alloc)(void *state, size_t size);

    /// Changes the size of an allocation, which may move it.
    ///
    /// @param state The state of the allocator
    /// @param ptr What was returned when it was allocated, never NULL
    /// @param old_size The size which it has
    /// @param new_size The size which it should have, never zero
    /// @returns The memory, or NULL if there is none. In that case, the
    ///   allocation is still there, and as it was.
    void *(*resize)(void *state, void *ptr, size_t old_size, size_t new_size);

    /// Releases an allocation.
    ///
    /// @param state The state of the allocator
    /// @param ptr What was returned when it was allocated, never NULL
    /// @param size The size which it has
    void (*free)(void *state, void *ptr, size_t size);

    /// Passed to all of the functions as it is.
    void *state;
} passgen_allocator;

/// Replaces the allocator which the library uses.
///
/// This is for all of the process, and not safe to do while another thread is
/// using the library. It has to be done before anything is allocated, or once
/// everything was released: what one allocator has allocated cannot be
/// released by another. The functions of the allocator have to be safe to
/// call from every thread which uses the library.
///
/// @param allocator The allocator to use, which is copied. NULL for the
///   default one.
PASSGEN_EXPORT void passgen_allocator_set(const passgen_allocator *allocator);
