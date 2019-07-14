/// @file tests.h
/// @author Patrick M. Elsen
/// @license MIT.
///
/// Small testing framework.

#pragma once
#include <stdbool.h>
#include <stdio.h>

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
} test_ret;

test_ret test_ok;

typedef test_ret test_fun(void);

typedef struct {
  const char *name;
  test_fun *fun;
} test_t;

bool run(test_t test);
#define test(id) \
  { .name = #id, .fun = test_##id }

#define assert(some)   \
  if (!(some))         \
    return (test_ret){ \
        .ok = false, .assertion = #some, .line = __LINE__, .func = __func__};

extern test_t tests[];
