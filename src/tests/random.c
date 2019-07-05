#include "passgen/pattern.h"
#include "passgen/random.h"
#include "util/tests.h"
#include <stdbool.h>
#include <stdio.h>
#include <time.h>

test_ret test_random_uint8(void);
test_ret test_random_uint8_max(void);
test_ret test_random_uint16(void);
test_ret test_random_uint16_max(void);

test_t tests[] = {
  test(random_uint8),
  test(random_uint8_max),
  test(random_uint16),
  test(random_uint16_max),
  {NULL, NULL}
};

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
