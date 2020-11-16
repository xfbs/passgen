#include <stddef.h>

#include "bench.h"

bench_declare(pattern_parse);
bench_declare(dummy_bench);

struct bench_item bench_dummy;
struct bench_item bench_pattern_parse;

struct bench_item items[] = {
    bench_define(pattern_parse),
    bench_define(dummy_bench),
    {NULL}};

struct bench_run bench_runs[] = {
  {.item = &bench_dummy, .batch = 1, .data = NULL},
  {.item = NULL},
};
