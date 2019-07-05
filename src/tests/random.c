#include "passgen/pattern.h"
#include "passgen/random.h"
#include <stdbool.h>
#include <stdio.h>
#include <time.h>

typedef struct {
  bool ok;
  const char *assertion;
  const char *func;
  size_t line;
} test_ret;

test_ret test_ok = {.ok = true};

typedef test_ret test_fun(void);

test_ret test_random_uint8(void);
test_ret test_random_uint8_max(void);
test_ret test_random_uint16(void);
test_ret test_random_uint16_max(void);

typedef struct {
  const char *name;
  test_fun *fun;
} test_t;

bool run(test_t test);
#define test(id) {.name = #id, .fun = test_ ## id}

#define assert(some) if(!(some)) return (test_ret) {.ok = false, .assertion = #some, .line = __LINE__, .func = __func__};

int main() {
  test_t tests[] = {
    test(random_uint8),
    test(random_uint8_max),
    test(random_uint16),
    test(random_uint16_max),
    {NULL, NULL}
  };

  size_t failures = 0;
  size_t success = 0;
  for(size_t i = 0; tests[i].name; ++i) {
    if(run(tests[i])) {
      success += 1;
    } else {
      failures += 1;
    }
  }

  printf("\033[1;34m=>\033[0m %zi/%zi tests passed.\n", success, success + failures);

  if(failures) {
    return -1;
  } else {
    return 0;
  }
}

bool run(test_t test) {
  clock_t before = clock();
  test_ret ret = test.fun();
  clock_t total = clock() - before;

  double time = total / (CLOCKS_PER_SEC * 1.0);

  if(ret.ok) {
    printf("%-20s \033[0;32mpassed\033[0m in %4.3lfs.\n", test.name, time);
  } else {
    printf("%-20s \033[0;31mfailed\033[0m in %4.3lfs.\n", test.name, time);
    printf("    \033[0;31m%s\033[0m failed at %s:%zi\n", ret.assertion, ret.func, ret.line);
  }

  return ret.ok;
}

test_ret test_random_uint8(void) {
  random_t *rand = random_new();
  assert(rand);

  // generate random nubers until we got almost all of them.
  bool gotten[UINT8_MAX] = {false};
  for (size_t i = 0; i < (32 * UINT8_MAX); ++i) {
    gotten[random_uint8(rand)] = true;
  }

  // there's still a (255/256)^(8*256) = 0.03% chance this fails.
  for (size_t i = 0; i < UINT8_MAX; ++i) {
    assert(gotten[i]);
  }

  random_close(rand);

  return test_ok;
}

test_ret test_random_uint8_max(void) {
  random_t *rand = random_new();
  assert(rand);

  for (size_t max = 1; max < UINT8_MAX; ++max) {
    // generate random nubers until we got almost all of them.
    bool gotten[UINT8_MAX] = {false};
    for (size_t i = 0; i < (16 * UINT8_MAX); ++i) {
      uint8_t r = random_uint8_max(rand, max);
      assert(r < max);
      gotten[r] = true;
    }

    // there's still a (255/256)^(8*256) = 0.03% chance this fails.
    for (size_t i = 0; i < max; ++i) {
      assert(gotten[i]);
    }
  }

  random_close(rand);

  return test_ok;
}

test_ret test_random_uint16(void) {
  random_t *rand = random_new();
  assert(rand);

  // generate random nubers until we got almost all of them.
  bool gotten[UINT16_MAX] = {false};
  for (size_t i = 0; i < (32 * UINT16_MAX); ++i) {
    gotten[random_uint16(rand)] = true;
  }

  // there's still a (255/256)^(8*256) = 0.03% chance this fails.
  for (size_t i = 0; i < UINT16_MAX; ++i) {
    assert(gotten[i]);
  }

  random_close(rand);

  return test_ok;
}

test_ret test_random_uint16_max(void) {
  random_t *rand = random_new();
  assert(rand);

  size_t max[] = {1, 2, 3, 4, 5, 100, 200, 500, 1000, 1500, 2000, 5000, 10000, 15000, 20000, 30000, 45000, 60000, 0};

  for (size_t n = 1; max[n]; ++n) {
    bool gotten[UINT16_MAX] = {false};
    for (size_t i = 0; i < (16 * max[n]); ++i) {
      uint16_t r = random_uint16_max(rand, max[n]);
      assert(r < max[n]);
      gotten[r] = true;
    }

    for (size_t i = 0; i < max[n]; ++i) {
      assert(gotten[i]);
    }
  }

  random_close(rand);

  return test_ok;
}
