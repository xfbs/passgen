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
#define passgen_assert(cond)                                          \
    do {                                                              \
        if(!(cond)) {                                                 \
            passgen_assert_fail(#cond, __FILE__, __func__, __LINE__); \
        }                                                             \
    } while(0)
#else
/// Assertion, disabled in release build.
#define passgen_assert(cond) (void) (cond)
#endif

/// Checks an argument which the caller of a public function passed, in every
/// build. If it is not valid, which is a mistake in the program which calls
/// the library, this prints what is wrong and aborts. Use this where carrying
/// on would hang or corrupt memory, and there is no way to return an error.
#define passgen_require(cond)                      \
    do {                                           \
        if(!(cond)) {                              \
            passgen_require_fail(#cond, __func__); \
        }                                          \
    } while(0)

/// Prints which argument is not valid, and aborts.
void passgen_require_fail(const char *cond, const char *func);

/// Prints assertion failure and optionally backtrace.
void passgen_assert_fail(
    const char *cond,
    const char *file,
    const char *func,
    size_t line);

/// Static assertion.
#define passgen_static_assert(x) _Static_assert(x, #x)
