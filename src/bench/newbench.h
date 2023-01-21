#pragma once
#include <stdbool.h>

/// Benchmark argument
typedef struct bench_arg {
    const char *name;
    const char *desc;
    const char *value;
} bench_arg;

/// Benchmark definition
typedef struct bench {
    const char *group;
    const char *name;
    const char *desc;
    bench_arg *args;
    bool consumes;
    void *(*prepare)(const char *args[]);
    void *(*iterate)(void *data);
    void (*cleanup)(void *data);
    void (*release)(void *data);
} bench;

extern const bench *benches[];
