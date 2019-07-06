#pragma once
#include <stdint.h>
#include <stdio.h>

#define RANDOM_H_BUFEN 250

typedef struct random_t {
  uint8_t buffer[RANDOM_H_BUFEN];
  size_t pos;
  FILE *device;
} random_t;

random_t *random_new();
random_t *random_new_full(const char *device);
random_t *random_open(random_t *random);
random_t *random_open_full(const char *device);

void random_close(random_t *random);
void random_free(random_t *random);

void random_read(random_t *random, void *dest, size_t bytes);

uint8_t random_uint8(random_t *random);
uint16_t random_uint16(random_t *random);
uint32_t random_uint32(random_t *random);
uint64_t random_uint64(random_t *random);

uint8_t random_uint8_max(random_t *random, uint8_t max);
uint16_t random_uint16_max(random_t *random, uint16_t max);
uint32_t random_uint32_max(random_t *random, uint32_t max);
uint64_t random_uint64_max(random_t *random, uint64_t max);
