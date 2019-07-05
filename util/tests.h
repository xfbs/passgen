#pragma once
#include <stdbool.h>
#include <stdio.h>

typedef struct {
  bool ok;
  const char *assertion;
  const char *func;
  size_t line;
} test_ret;

test_ret test_ok;

typedef test_ret test_fun(void);

typedef struct {
  const char *name;
  test_fun *fun;
} test_t;

bool run(test_t test);
#define test(id) {.name = #id, .fun = test_ ## id}

#define assert(some) if(!(some)) return (test_ret) {.ok = false, .assertion = #some, .line = __LINE__, .func = __func__};

extern test_t tests[];
