#include "passgen/random.h"
#include "passgen/pattern.h"
#include "tests.h"
#include <stdbool.h>
#include <stdio.h>
#include <time.h>

test_result test_random_uint8(void) {
  random_t *rand = random_new();
  assert(rand);

  // generate random nubers until we got almost all of them.
  bool gotten[UINT8_MAX + 1] = {false};
  for(size_t i = 0; i < (32 * UINT8_MAX); ++i) {
    gotten[random_uint8(rand)] = true;
  }

  // there's still a (255/256)^(8*256) = 0.03% chance this fails.
  for(size_t i = 0; i < UINT8_MAX; ++i) {
    assert(gotten[i]);
  }

  random_free(rand);

  return test_ok;
}

test_result test_random_uint8_max(void) {
  random_t *rand = random_new();
  assert(rand);

  for(size_t max = 1; max < UINT8_MAX; ++max) {
    // generate random nubers until we got almost all of them.
    bool gotten[UINT8_MAX] = {false};
    for(size_t i = 0; i < (16 * UINT8_MAX); ++i) {
      uint8_t r = random_uint8_max(rand, max);
      assert(r < max);
      gotten[r] = true;
    }

    // there's still a (255/256)^(8*256) = 0.03% chance this fails.
    for(size_t i = 0; i < max; ++i) {
      assert(gotten[i]);
    }
  }

  random_free(rand);

  return test_ok;
}

test_result test_random_uint16(void) {
  random_t *rand = random_new();
  assert(rand);

  // generate random nubers until we got almost all of them.
  bool gotten[UINT16_MAX + 1] = {false};
  for(size_t i = 0; i < (32 * UINT16_MAX); ++i) {
    gotten[random_uint16(rand)] = true;
  }

  // there's still a (255/256)^(8*256) = 0.03% chance this fails.
  for(size_t i = 0; i < UINT16_MAX; ++i) {
    assert(gotten[i]);
  }

  random_free(rand);

  return test_ok;
}

test_result test_random_uint16_max(void) {
  random_t *rand = random_new();
  assert(rand);

  size_t max[] = {
      1,
      2,
      3,
      4,
      5,
      100,
      200,
      500,
      1000,
      1500,
      2000,
      5000,
      10000,
      15000,
      20000,
      30000,
      45000,
      60000,
      0};

  for(size_t n = 1; max[n]; ++n) {
    bool gotten[UINT16_MAX] = {false};
    for(size_t i = 0; i < (16 * max[n]); ++i) {
      uint16_t r = random_uint16_max(rand, max[n]);
      assert(r < max[n]);
      gotten[r] = true;
    }

    for(size_t i = 0; i < max[n]; ++i) {
      assert(gotten[i]);
    }
  }

  random_free(rand);

  return test_ok;
}

test_result test_random_uint32_max(void) {
  random_t *rand = random_new();
  assert(rand);

  for(size_t max = 1; max <= UINT32_MAX; max += UINT16_MAX) {
    assert(random_uint32_max(rand, max) < max);
  }

  random_free(rand);

  return test_ok;
}

test_result test_random_uint64_max(void) {
  random_t *rand = random_new();
  assert(rand);

  for(size_t max = 1; max <= (UINT64_MAX >> 1); max += 1) {
    assert(random_uint64_max(rand, max) < max);
    max = 1.001 * max;
  }

  random_free(rand);

  return test_ok;
}

test_result test_random_new(void) {
  random_t *random = random_new();
  assert(random);
  assert(random->read);
  random_free(random);

  return test_ok;
}

test_result test_random_new_path(void) {
  random_t *random;
  random = random_new_path("/dev/nonexistent");
  assert(!random);

  random = random_new_path("/dev/zero");
  assert(random);
  assert(random->data);
  assert(random_uint8(random) == 0);
  assert(random_uint16(random) == 0);
  assert(random_uint32(random) == 0);
  assert(random_uint64(random) == 0);
  random_free(random);

  return test_ok;
}

test_result test_random_open(void) {
  random_t random;
  assert(random_open(&random));
  assert(random.read);
  random_close(&random);
  assert(!random.read);

  return test_ok;
}

test_result test_random_open_path(void) {
  random_t random;
  assert(!random_open_path(&random, "/dev/nonexistent"));

  assert(random_open_path(&random, "/dev/zero"));
  assert(random.data);
  assert(random_uint8(&random) == 0);
  assert(random_uint16(&random) == 0);
  assert(random_uint32(&random) == 0);
  assert(random_uint64(&random) == 0);
  random_close(&random);
  assert(!random.data);

  return test_ok;
}

test_result test_random_read(void) {
  random_t random;
  assert(random_open(&random));

  uint8_t data[2000] = {0};

  // fill small.
  random_read(&random, &data[0], 1);
  assert(random.pos == 1);
  assert(data[0] == random.buffer[0]);
  assert(data[1] == 0);

  random_read(&random, &data[0], 2);
  assert(random.pos == 3);
  assert(data[0] == random.buffer[1]);
  assert(data[1] == random.buffer[2]);
  assert(data[2] == 0);

  random_read(&random, &data[0], 4);
  assert(random.pos == 7);
  assert(data[0] == random.buffer[3]);
  assert(data[1] == random.buffer[4]);
  assert(data[2] == random.buffer[5]);
  assert(data[3] == random.buffer[6]);
  assert(data[4] == 0);

  random_read(&random, &data[0], 8);
  assert(random.pos == 15);
  assert(data[0] == random.buffer[7]);
  assert(data[1] == random.buffer[8]);
  assert(data[2] == random.buffer[9]);
  assert(data[3] == random.buffer[10]);
  assert(data[4] == random.buffer[11]);
  assert(data[5] == random.buffer[12]);
  assert(data[6] == random.buffer[13]);
  assert(data[7] == random.buffer[14]);
  assert(data[8] == 0);

  // test wraparound.
  while(random.pos != (sizeof(random.buffer) - 1)) {
    random_read(&random, &data[0], 1);
  }
  random_read(&random, &data[0], 1);
  assert(random.pos == 0);

  while(random.pos != (sizeof(random.buffer) - 1)) {
    random_read(&random, &data[0], 1);
  }
  random_read(&random, &data[0], 3);
  assert(data[1] == random.buffer[0]);
  assert(data[2] == random.buffer[1]);
  assert(random.pos == 2);

  // test reading larger.
  random_read(&random, &data[0], sizeof(random.buffer) + 1);
  assert(random.pos == 2);

  random_close(&random);

  return test_ok;
}
