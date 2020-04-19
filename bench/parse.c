#include "bench.h"

bench_new(pattern_parse, "Parses a pattern using pattern_parse().");

void *bench_pattern_parse_setup(const char *data) {
  if(data) {
    return (void *)data;
  } else {
    return "abc";
  }
}

void bench_pattern_parse_func(void *data) {
  (void) data;
  /*
  const char *str = data;

  pattern_t *pattern = pattern_parse(&str);
  assert(pattern);
  pattern_free(pattern);
  */
}

void bench_pattern_parse_free(void *state) {
  (void) state;
}
