#include "passgen/random.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#define check(ptr)                                                             \
  if (ptr == NULL)                                                             \
  goto error

static const char *random_default_device = "/dev/urandom";

void random_reload(random_t *random) {
  assert(random != NULL);
  // printf("pre-reload pos: %zi\n", random->pos);

  // FIXME: don't overwrite everything.
  size_t bytes = fread(&random->buffer, sizeof(random->buffer[0]),
                       sizeof(random->buffer), random->device);

  assert(bytes == sizeof(random->buffer));

  random->pos = 0;

  // printf("post-reload pos: %zi, read: %zi\n", random->pos, bytes);
}

uint8_t *random_extract(random_t *random, size_t bytes) {
  assert(random != NULL);
  // printf("pre-extract pos: %zi, bytes: %zi\n", random->pos, bytes);

  assert(bytes < sizeof(random->buffer));

  if (random->pos >= (sizeof(random->buffer) - bytes)) {
    random_reload(random);
  }

  assert(random->pos <= (sizeof(random->buffer) - bytes));

  random->pos += bytes;

  assert(random->pos < sizeof(random->buffer));
  assert(random->pos >= bytes);

  return &random->buffer[random->pos - bytes];
}

random_t *random_new() { return random_new_full(random_default_device); }

random_t *random_new_full(const char *device) {
  random_t *random = malloc(sizeof(random_t));
  check(random);

  random->device = fopen(device, "r");
  check(random->device);

  random_reload(random);

  return random;

error:
  return NULL;
}

void random_close(random_t *random) {
  if (random != NULL) {
    fclose(random->device);
  }

  free(random);
}

uint8_t random_uint8(random_t *random) {
  uint8_t *data = random_extract(random, sizeof(uint8_t));
  return *data;
}

uint16_t random_uint16(random_t *random) {
  uint8_t *data = random_extract(random, sizeof(uint16_t));
  uint16_t num;
  memcpy(&num, data, sizeof(uint16_t));
  return num;
}

uint32_t random_uint32(random_t *random) {
  uint8_t *data = random_extract(random, sizeof(uint32_t));
  uint32_t num;
  memcpy(&num, data, sizeof(uint32_t));
  return num;
}

uint64_t random_uint64(random_t *random) {
  uint8_t *data = random_extract(random, sizeof(uint64_t));
  uint64_t num;
  memcpy(&num, data, sizeof(uint64_t));
  return num;
}

uint8_t random_uint8_max(random_t *random, uint8_t max) {
  uint8_t num;

  // FIXME: strip higher bits?
  do {
    num = random_uint8(random);
  } while (num >= max);

  return num;
}

uint16_t random_uint16_max(random_t *random, uint16_t max) {
  if (max <= UINT8_MAX) {
    return random_uint8_max(random, max);
  }

  uint16_t num;

  do {
    num = random_uint16(random);
  } while (num >= max);

  return num;
}

uint32_t random_uint32_max(random_t *random, uint32_t max) {
  if (max < UINT16_MAX) {
    return random_uint16_max(random, max);
  }

  uint32_t num;

  do {
    num = random_uint32(random);
  } while (num >= max);

  return num;
}

uint64_t random_uint64_max(random_t *random, uint64_t max) {
  if (max < UINT32_MAX) {
    return random_uint32_max(random, max);
  }

  uint64_t num;

  do {
    num = random_uint64(random);
  } while (num >= max);

  return num;
}
