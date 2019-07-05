#include "passgen/random.h"
#include <stdio.h>
#include <time.h>

typedef void bench_func(void *data, size_t ops);
void bench_random_uint8();
void bench_random_uint16();
void bench_random_uint32();
void bench_random_uint64();

void bench_random_uint8_max();
void bench_random_uint16_max();
void bench_random_uint32_max();
void bench_random_uint64_max();

void bench(const char *name, bench_func func, void *data, size_t count);

int main() {
  random_t *rand = random_new();
  bench("random_uint8", &bench_random_uint8, rand, 10000000);
  bench("random_uint16", &bench_random_uint16, rand, 10000000);
  bench("random_uint32", &bench_random_uint32, rand, 5000000);
  bench("random_uint64", &bench_random_uint64, rand, 2000000);

  bench("random_uint8_max", &bench_random_uint8_max, rand, 10000000);
  bench("random_uint16_max", &bench_random_uint16_max, rand, 10000000);
  bench("random_uint32_max", &bench_random_uint32_max, rand, 100000);
  bench("random_uint64_max", &bench_random_uint64_max, rand, 100000);

  random_close(rand);
  return 0;
}

void bench_random_uint8(void *data, size_t count) {
  random_t *rand = data;
  volatile uint8_t sum = 0;

  for(size_t i = 0; i < count; i++) {
    sum += random_uint8(rand);
  }
}

void bench_random_uint8_max(void *data, size_t count) {
  random_t *rand = data;
  volatile uint8_t sum = 0;

  for(size_t i = 0; i < count; i++) {
    sum += random_uint8_max(rand, 213);
  }
}

void bench_random_uint16(void *data, size_t count) {
  random_t *rand = data;
  volatile uint16_t sum = 0;

  for(size_t i = 0; i < count; i++) {
    sum += random_uint16(rand);
  }
}

void bench_random_uint16_max(void *data, size_t count) {
  random_t *rand = data;
  volatile uint16_t sum = 0;

  for(size_t i = 0; i < count; i++) {
    sum += random_uint16_max(rand, 10000);
  }
}

void bench_random_uint32_max(void *data, size_t count) {
  random_t *rand = data;
  volatile uint32_t sum = 0;

  for(size_t i = 0; i < count; i++) {
    sum += random_uint32_max(rand, 128924398);
  }
}

void bench_random_uint64_max(void *data, size_t count) {
  random_t *rand = data;
  volatile uint64_t sum = 0;

  for(size_t i = 0; i < count; i++) {
    sum += random_uint64_max(rand, 10000000000);
  }
}

void bench_random_uint32(void *data, size_t count) {
  random_t *rand = data;
  volatile uint32_t sum = 0;

  for(size_t i = 0; i < count; i++) {
    sum += random_uint32(rand);
  }
}

void bench_random_uint64(void *data, size_t count) {
  random_t *rand = data;
  volatile uint64_t sum = 0;

  for(size_t i = 0; i < count; i++) {
    sum += random_uint64(rand);
  }
}

void bench(const char *name, bench_func func, void *data, size_t count) {
  // do benchmark
  clock_t before = clock();
  func(data, count);
  clock_t after = clock();

  // calculate
  double time = (after - before) / (CLOCKS_PER_SEC / 1.0);
  double ops = count / time;
  const char *prefix = "";

  // make it human readable.
  if(ops > 1000000) {
    ops /= 1000000;
    prefix = "M";
  } else if(ops > 1000) {
    ops /= 1000;
    prefix = "K";
  }

  printf("%-25s %6.2lf %sops/s (in %0.2lfs)\n", name, ops, prefix, time);
}
