#include <stddef.h>

#include "bench.h"

bench_declare(pattern_parse);

struct bench_item items[] = {
  bench_define(pattern_parse),
  bench_define(pattern_parse),
  bench_define(pattern_parse),
  bench_define(pattern_parse),
  bench_define(pattern_parse),
  bench_define(pattern_parse),
  bench_define(pattern_parse),
  bench_define(pattern_parse),
  bench_define(pattern_parse),
  {NULL}
};
