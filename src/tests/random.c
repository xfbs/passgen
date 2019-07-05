#include "passgen/pattern.h"
#include "passgen/random.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

void test_random_uint8();
void test_random_uint8_max();
void test_random_uint16();
void test_random_uint16_max();

int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;

  test_random_uint8();
  test_random_uint8_max();
  test_random_uint16();
  // too slow.
  //test_random_uint16_max();

  fprintf(stdout, "all tests passed.\n");
  return 0;
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

  for (size_t max = 1; max < UINT16_MAX; ++max) {
    printf("loopy %i.\n", max);
    bool gotten[UINT16_MAX] = {false};
    for (size_t i = 0; i < (16 * max); ++i) {
      uint16_t r = random_uint16_max(rand, max);
      assert(r < max);
      gotten[r] = true;
    }

    for (size_t i = 0; i < max; ++i) {
      assert(gotten[i]);
    }
  }

  random_close(rand);
}
