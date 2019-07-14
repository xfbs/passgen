#include "passgen/random.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#define check(ptr) \
  if(ptr == NULL) goto error

static const char *random_default_device = "/dev/urandom";

void random_reload(random_t *random) {
  assert(random != NULL);

  // read random data.
  size_t bytes = fread(
      &random->buffer,
      sizeof(random->buffer[0]),
      sizeof(random->buffer),
      random->device);

  // make sure we've read enough.
  assert(bytes == sizeof(random->buffer));

  // reset position in ring buffer.
  random->pos = 0;
}

void random_read(random_t *random, void *data, size_t bytes) {
  if(bytes <= sizeof(random->buffer)) {
    // maximum bytes we can have right now
    size_t left = sizeof(random->buffer) - random->pos;

    if(bytes < left) {
      // if we have enough, just copy it over.
      memcpy(data, &random->buffer[random->pos], bytes);
      random->pos += bytes;
    } else {
      // if we don't have enough, copy over whatever we have and reload.
      memcpy(data, random->buffer, left);
      random_reload(random);

      // if there's more to be read, recurse.
      if(bytes != left) {
        random_read(random, data + left, bytes - left);
      }
    }
  } else {
    assert(fread(data, 1, bytes, random->device) == bytes);
  }
}

random_t *random_new() {
  return random_new_path(random_default_device);
}

random_t *random_new_path(const char *path) {
  random_t *random = malloc(sizeof(random_t));
  if(!random) return NULL;

  if(!random_open_path(random, path)) {
    free(random);
    return NULL;
  }

  return random;
}

random_t *random_new_file(FILE *file) {
  random_t *random = malloc(sizeof(random_t));
  if(!random) return NULL;

  random_open_file(random, file);

  return random;
}

random_t *random_open(random_t *random) {
  return random_open_path(random, random_default_device);
}

random_t *random_open_path(random_t *random, const char *path) {
  FILE *device = fopen(path, "r");
  if(!device) return NULL;

  return random_open_file(random, device);

  random_reload(random);

  return random;
}

random_t *random_open_file(random_t *random, FILE *file) {
  random->device = file;
  random_reload(random);
  return random;
}

void random_close(random_t *random) {
  if(random != NULL) {
    fclose(random->device);
    random->device = NULL;
  }
}

void random_free(random_t *random) {
  if(random != NULL) {
    fclose(random->device);
    random->device = NULL;
  }

  free(random);
}

uint8_t random_uint8(random_t *random) {
  uint8_t data;
  random_read(random, &data, sizeof(data));
  return data;
}

uint16_t random_uint16(random_t *random) {
  uint16_t data;
  random_read(random, &data, sizeof(data));
  return data;
}

uint32_t random_uint32(random_t *random) {
  uint32_t data;
  random_read(random, &data, sizeof(data));
  return data;
}

uint64_t random_uint64(random_t *random) {
  uint64_t data;
  random_read(random, &data, sizeof(data));
  return data;
}

uint8_t random_uint8_max(random_t *random, uint8_t max) {
  uint8_t mask = max;
  mask |= mask >> 4;
  mask |= mask >> 2;
  mask |= mask >> 1;

  uint8_t num;
  do {
    num = random_uint8(random) & mask;
  } while(num >= max);

  return num;
}

uint16_t random_uint16_max(random_t *random, uint16_t max) {
  if(max <= UINT8_MAX) {
    return random_uint8_max(random, max);
  }

  uint16_t mask = max;
  mask |= mask >> 8;
  mask |= mask >> 4;
  mask |= mask >> 2;
  mask |= mask >> 1;

  uint16_t num;

  do {
    num = random_uint16(random) & mask;
  } while(num >= max);

  return num;
}

uint32_t random_uint32_max(random_t *random, uint32_t max) {
  if(max < UINT16_MAX) {
    return random_uint16_max(random, max);
  }

  uint32_t mask = max;
  mask |= mask >> 16;
  mask |= mask >> 8;
  mask |= mask >> 4;
  mask |= mask >> 2;
  mask |= mask >> 1;

  uint32_t num;

  do {
    num = random_uint32(random) & mask;
  } while(num >= max);

  return num;
}

uint64_t random_uint64_max(random_t *random, uint64_t max) {
  if(max < UINT32_MAX) {
    return random_uint32_max(random, max);
  }

  uint64_t mask = max;
  mask |= mask >> 32;
  mask |= mask >> 16;
  mask |= mask >> 8;
  mask |= mask >> 4;
  mask |= mask >> 2;
  mask |= mask >> 1;

  uint64_t num;

  do {
    num = random_uint64(random) & mask;
  } while(num >= max);

  return num;
}
