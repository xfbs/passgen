/// @file assert.h
/// @author Patrick M. Elsen
/// @brief Assertions for debug builds.
#pragma once
#include <passgen/config.h>
#include <stddef.h>

#ifdef PASSGEN_DEBUG
/// Assertion, enabled in debug build.
///
/// This is wrapped in a loop which runs once, so that it is a single statement
/// wherever it is used. As a bare `if`, an `else` following it would have
/// ended up belonging to the assertion.
#define passgen_assert(cond)                                              \
    do {                                                                  \
        if(!(cond)) {                                                     \
            passgen_assert_fail(#cond, __FILE__, __func__, __LINE__);     \
        }                                                                 \
    } while(0)
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
#define passgen_static_assert(x) _Static_assert(x, #x)
