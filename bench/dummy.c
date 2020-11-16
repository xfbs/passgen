#include "bench.h"

bench_new(dummy_bench, "Parses a pattern using pattern_parse().");

void *bench_dummy_bench_setup(const char *data) {
  return (void *) data;
}

void bench_dummy_bench_func(void *data) {
  (void) data;
}

void bench_dummy_bench_free(void *state) {
  (void) state;
}

struct bench_item bench_dummy = {
  .name = "dummy bench",
  .info = "does nothing",
  .setup = &bench_dummy_bench_setup,
  .func = &bench_dummy_bench_func,
  .free = &bench_dummy_bench_free,
};
