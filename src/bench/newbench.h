#pragma once
#include <stdbool.h>
#include <passgen/util/hashmap.h>

/// Benchmark definition
typedef struct bench {
    const char *group;
    const char *name;
    const char *desc;
    bool consumes;
    void *(*prepare)(const passgen_hashmap *opts);
    void *(*iterate)(void *data);
    void (*cleanup)(void *data);
    void (*release)(void *data);
} bench;

extern const bench *benches[];
