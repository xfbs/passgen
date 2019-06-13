#include "random.h"
#include <stdlib.h>
#include <assert.h>
#define check(ptr) if(ptr == NULL) goto error

static const char *random_default_device = "/dev/urandom";


random_t *random_new() {
  return random_new_full(random_default_device);
}

random_t *random_new_full(const char *device) {
  random_t *random = malloc(sizeof(random_t));
  check(random);

  random->device = fopen(device, "r");
  check(random->device);

  size_t bytes = fread(&random->buffer, sizeof(random->buffer), sizeof(random->buffer[0]), random->device);
  assert(bytes == sizeof(random->buffer));

  random->pos = 0;

  return random;

error:
  return NULL;
}

void random_close(random_t *random) {
  if(random != NULL) {
    fclose(random->device);
  }
}

uint8_t  random_uint8 (random_t *random);
uint16_t random_uint16(random_t *random);
uint32_t random_uint32(random_t *random);
uint64_t random_uint64(random_t *random);

uint8_t  random_uint8_max (random_t *random, uint8_t max);
uint16_t random_uint16_max(random_t *random, uint16_t max);
uint32_t random_uint32_max(random_t *random, uint32_t max);
uint64_t random_uint64_max(random_t *random, uint64_t max);
