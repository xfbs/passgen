#include "passgen/pronounceable.h"
#include "passgen/markov.h"
#include "passgen/markov_data.h"
#include "tests.h"
#define SEED 328543091702ULL

test_result test_pronounceable(void) {
  passgen_random_t *rand = passgen_random_new_xorshift(SEED);
  assert(rand);

  for(size_t i = 0; i < 1000; i++) {
    int32_t buffer[64];
    size_t ret =
        passgen_pronounceable(PASSGEN_PRONOUNCEABLE_ENGLISH, rand, buffer, 64);

    assert(ret > 0);
    assert(ret <= sizeof(buffer));
  }

  passgen_random_free(rand);

  return test_ok;
}

/* rough test to see that there is a reasonable length distribution when
 * generating english words. */
test_result test_pronounceable_englendist(void) {
  passgen_random_t *rand = passgen_random_new_xorshift(SEED);
  assert(rand);
  size_t lens[64] = {0};

  for(size_t i = 0; i < 1000; i++) {
    int32_t buffer[64];
    size_t ret =
        passgen_pronounceable(PASSGEN_PRONOUNCEABLE_ENGLISH, rand, buffer, 64);

    assert(ret <= sizeof(buffer));
    lens[ret]++;
  }

  assert(lens[0] == 0);
  assert(lens[2] > 10);
  assert(lens[3] > 10);
  assert(lens[4] > 10);
  assert(lens[5] > 10);
  assert(lens[5] > 10);
  assert(lens[6] > 10);
  assert(lens[7] > 10);
  assert(lens[8] > 10);
  assert(lens[9] > 10);
  assert(lens[10] > 10);

  passgen_random_free(rand);

  return test_ok;
}

/* rough test to see that there is a reasonable length distribution when
 * generating english words. */
test_result test_pronounceable_latlendist(void) {
  passgen_random_t *rand = passgen_random_new_xorshift(SEED);
  assert(rand);
  size_t lens[64] = {0};

  for(size_t i = 0; i < 1000; i++) {
    int32_t buffer[64];
    size_t ret =
        passgen_pronounceable(PASSGEN_PRONOUNCEABLE_LATIN, rand, buffer, 64);

    assert(ret > 0);
    assert(ret <= sizeof(buffer));

    lens[ret]++;
  }

  assert(lens[0] == 0);
  assert(lens[2] > 10);
  assert(lens[3] > 10);
  assert(lens[4] > 10);
  assert(lens[5] > 10);
  assert(lens[5] > 10);
  assert(lens[6] > 10);
  assert(lens[7] > 10);
  assert(lens[8] > 10);
  assert(lens[9] > 10);
  assert(lens[10] > 10);

  passgen_random_free(rand);

  return test_ok;
}

test_result test_pronounceable_len(void) {
  passgen_random_t *rand = passgen_random_new_xorshift(SEED);
  assert(rand);

  for(size_t i = 0; i < 1000; i++) {
    int32_t buffer[11];
    int ret = passgen_pronounceable_len(
        PASSGEN_PRONOUNCEABLE_ENGLISH,
        rand,
        buffer,
        8,
        11,
        0);

    assert(8 <= ret);
    assert(ret <= 11);
  }

  for(size_t i = 0; i < 1000; i++) {
    int32_t buffer[128];
    int ret = passgen_pronounceable_len(
        PASSGEN_PRONOUNCEABLE_ENGLISH,
        rand,
        buffer,
        100,
        120,
        10);

    assert(0 == ret);
  }

  passgen_random_free(rand);

  return test_ok;
}

test_result test_pronounceable_engascii(void) {
  passgen_random_t *rand = passgen_random_new_xorshift(SEED);
  assert(rand);

  for(size_t i = 0; i < 1000; i++) {
    int32_t buffer[64];
    size_t ret =
        passgen_pronounceable(PASSGEN_PRONOUNCEABLE_ENGLISH, rand, buffer, 64);

    for(size_t i = 0; i < ret; i++) {
      assert('a' <= buffer[i]);
      assert(buffer[i] <= 'z');
    }
  }

  passgen_random_free(rand);

  return test_ok;
}

test_result test_pronounceable_minascii(void) {
  passgen_random_t *rand = passgen_random_new_xorshift(SEED);
  assert(rand);

  for(size_t i = 0; i < 1000; i++) {
    int32_t buffer[64];
    size_t ret = passgen_pronounceable_len(
        PASSGEN_PRONOUNCEABLE_ENGLISH,
        rand,
        buffer,
        7,
        10,
        0);

    for(size_t i = 0; i < ret; i++) {
      assert('a' <= buffer[i]);
      assert(buffer[i] <= 'z');
    }
  }

  passgen_random_free(rand);

  return test_ok;
}

test_result test_pronounceable_lendist(void) {
  /* make sure that when specifying lengths [8, 11], you really can get all
   * possible lengths. */
  passgen_random_t *rand = passgen_random_new_xorshift(SEED);
  assert(rand);

  size_t repeat = 1000;
  size_t lens[4] = {0, 0, 0, 0};

  for(size_t i = 0; i < repeat; i++) {
    int32_t buffer[11];
    size_t ret = passgen_pronounceable_len(
        PASSGEN_PRONOUNCEABLE_ENGLISH,
        rand,
        buffer,
        8,
        11,
        0);

    assert(8 <= ret);
    assert(ret <= 11);

    lens[ret - 8] += 1;
  }

  assert(80 < lens[0]);
  assert(80 < lens[1]);
  assert(80 < lens[2]);
  assert(80 < lens[3]);

  passgen_random_free(rand);

  return test_ok;
}
