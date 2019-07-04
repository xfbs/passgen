#include <stdio.h>
#include <assert.h>
#include <stdbool.h>
#include "random.h"
#include "pattern.h"

void test_random();
void test_random_uint8();
void test_random_uint8_max();
void test_pattern();

int main(int argc, char *argv[]) {
  test_random();
  test_pattern();

  fprintf(stdout, "all tests passed.\n");
  return 0;
}

void test_random() {
  test_random_uint8();
  test_random_uint8_max();
}

void test_pattern() {
}

void test_random_uint8() {
  random_t *rand = random_new();
  assert(rand);

  // generate random nubers until we got almost all of them.
  bool gotten[UINT8_MAX] = {false};
  for(size_t i = 0; i < (32 * UINT8_MAX); ++i) {
    gotten[random_uint8(rand)] = true;
  }

  // there's still a (255/256)^(8*256) = 0.03% chance this fails.
  for(size_t i = 0; i < UINT8_MAX; ++i) {
    assert(gotten[i]);
  }

  random_close(rand);
}

void test_random_uint8_max() {
  random_t *rand = random_new();
  assert(rand);

  for(size_t max = 1; max < UINT8_MAX; ++max) {
    // generate random nubers until we got almost all of them.
    bool gotten[UINT8_MAX] = {false};
    for(size_t i = 0; i < (16 * UINT8_MAX); ++i) {
      uint8_t r = random_uint8_max(rand, max);
      assert(r <= max);
      gotten[r] = true;
    }

    // there's still a (255/256)^(8*256) = 0.03% chance this fails.
    for(size_t i = 0; i < max; ++i) {
      assert(gotten[i]);
    }
  }

  random_close(rand);
}
