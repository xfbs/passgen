/// @file assert.h
/// @author Patrick M. Elsen
/// @brief Assertions for debug builds.
#pragma once
#include <passgen/config.h>
#include <stddef.h>

#ifdef PASSGEN_DEBUG
/// Assertion, enabled in debug build.
#define passgen_assert(cond) \
    if(!(cond)) passgen_assert_fail(#cond, __FILE__, __func__, __LINE__)
#else
/// Assertion, disabled in release build.
#define passgen_assert(cond) (void) (cond)
#endif

/// Prints assertion failure and optionally backtrace.
void passgen_assert_fail(
    const char *cond,
    const char *file,
    const char *func,
    size_t line);

/// Static assertion.
#define passgen_static_assert(x) _Static_assert(x)
