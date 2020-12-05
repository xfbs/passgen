#include <stddef.h>

#include "bench.h"

extern struct bench_item bench_dummy;
extern struct bench_item bench_pattern_parse;

struct bench_item items[] = {
    {NULL}};

struct bench_run bench_runs[] = {
    {.item = &bench_dummy, .batch = 1, .data = NULL},
    {.item = NULL},
};
