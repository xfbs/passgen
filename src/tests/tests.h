/// @file tests.h
/// @author Patrick M. Elsen
/// @license MIT.
///
/// Small testing framework.

#pragma once
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/// Result of a test.
typedef struct {
    /// True if the test worked okay, false if there was an error.
    bool ok;
    /// If false, a string of the assertion that failed.
    const char *assertion;
    /// If false, the name of the function in which the failure happened.
    const char *func;
    /// The line in which the assertion failed.
    size_t line;
} test_result;

extern test_result test_ok;

typedef test_result test_func(void);

typedef struct {
    const char *name;
    test_func *func;
} test_entry;

typedef struct {
    test_entry entry;
    int number;
    int amount;
    int verbosity;
} test_run;

bool run(test_run test, int width);

#define test(id) \
    { .name = #id, .func = test_##id }

#define assert(some)            \
    if(!(some))                 \
        return (test_result){   \
            .ok = false,        \
            .assertion = #some, \
            .line = __LINE__,   \
            .func = __func__};

#define assert_eq(lhs, rhs)                \
    if(lhs != rhs)                         \
        return (test_result){              \
            .ok = false,                   \
            .assertion = #lhs " == " #rhs, \
            .line = __LINE__,              \
            .func = __func__};

#define assert_ne(lhs, rhs)                \
    if(lhs == rhs)                         \
        return (test_result){              \
            .ok = false,                   \
            .assertion = #lhs " != " #rhs, \
            .line = __LINE__,              \
            .func = __func__};

#define assert_gt(lhs, rhs)               \
    if(lhs <= rhs)                        \
        return (test_result){             \
            .ok = false,                  \
            .assertion = #lhs " > " #rhs, \
            .line = __LINE__,             \
            .func = __func__};

#define assert_ge(lhs, rhs)                \
    if(lhs < rhs)                          \
        return (test_result){              \
            .ok = false,                   \
            .assertion = #lhs " >= " #rhs, \
            .line = __LINE__,              \
            .func = __func__};

#define assert_lt(lhs, rhs)               \
    if(lhs >= rhs)                        \
        return (test_result){             \
            .ok = false,                  \
            .assertion = #lhs " < " #rhs, \
            .line = __LINE__,             \
            .func = __func__};

#define assert_le(lhs, rhs)                \
    if(lhs > rhs)                          \
        return (test_result){              \
            .ok = false,                   \
            .assertion = #lhs " <= " #rhs, \
            .line = __LINE__,              \
            .func = __func__};

#define assert_streq(lhs, rhs)             \
    if(strcmp(lhs, rhs) != 0)              \
        return (test_result){              \
            .ok = false,                   \
            .assertion = #lhs " == " #rhs, \
            .line = __LINE__,              \
            .func = __func__};

extern test_entry tests[];
extern size_t tests_len;
