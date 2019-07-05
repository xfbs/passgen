#include "passgen/pattern.h"
#include "passgen/random.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>

typedef void test_fun(void);

void test_random_uint8();
void test_random_uint8_max();
void test_random_uint16();
void test_random_uint16_max();

typedef struct {
  const char *name;
  test_fun *fun;
} test_t;

void run(test_t test);
#define test(id) {.name = #id, .fun = test_ ## id}

int main() {
  test_t tests[] = {
    test(random_uint8),
    test(random_uint8_max),
    test(random_uint16),
    test(random_uint16_max),
    {NULL, NULL}
  };

  for(size_t i = 0; tests[i].name; ++i) {
    run(tests[i]);
  }

  return 0;
}

void run(test_t test) {
  clock_t before = clock();
  test.fun();
  clock_t total = clock() - before;

  double time = total / (CLOCKS_PER_SEC * 1.0);
  printf("%-20s passed in %4.3lfs.\n", test.name, time);
}

void test_random_uint8() {
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
}

void test_random_uint8_max() {
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
}

void test_random_uint16() {
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
}

void test_random_uint16_max() {
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
}
