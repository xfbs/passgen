/// @file testalloc.h
///
/// An allocator for the tests, which can be made to fail, and which checks
/// that allocations are released with the size they were allocated with.
#pragma once
#include <passgen/allocator.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/// Never fail.
#define TEST_ALLOCATOR_NEVER SIZE_MAX

typedef struct {
    /// Number of allocations (including resizes) which were asked for.
    size_t requests;
    /// The request, counting from zero, which fails, and every one after it.
    /// @ref TEST_ALLOCATOR_NEVER for none.
    size_t fail_at;
    /// Number of allocations which have not been released.
    size_t live;
    /// Number of bytes which have not been released.
    size_t live_bytes;
    /// Number of times that a size was passed in which is not the size the
    /// allocation has.
    size_t mismatches;
} test_allocator;

/// Makes the library use the test allocator, starting from nothing.
void test_allocator_install(test_allocator *allocator, size_t fail_at);

/// Makes the library use its default allocator again.
void test_allocator_remove(void);

/// Whether everything was released, with the right sizes.
bool test_allocator_clean(const test_allocator *allocator);
