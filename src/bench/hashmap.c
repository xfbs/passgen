#include "bench.h"
#include <passgen/container/hashmap.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct hashmap_data {
    passgen_hashmap map;
    char **data;
    size_t count;
} hashmap_data;

static double hashmap_mult(void *raw_data) {
    hashmap_data *data = raw_data;
    return data->count;
}

static void *hashmap_prepare(const passgen_hashmap *opts) {
    (void) opts;
    hashmap_data *data = malloc(sizeof(struct hashmap_data));
    data->count = 10000;
    data->data = calloc(sizeof(const char *), data->count);
    for(size_t i = 0; i < data->count; i++) {
        data->data[i] = malloc(16);
        sprintf(data->data[i], "item-%zu", i);
    }
    passgen_hashmap_init(&data->map, &passgen_hashmap_context_default);
    return data;
}

static void hashmap_cleanup(void *raw_data) {
    hashmap_data *data = raw_data;
    passgen_hashmap_free(&data->map);

    for(size_t i = 0; i < data->count; i++) {
        free(data->data[i]);
    }

    free(data->data);
    free(data);
}

static void *bench_hashmap_insert(void *raw_data) {
    hashmap_data *data = raw_data;

    for(size_t i = 0; i < data->count; i++) {
        passgen_hashmap_insert(&data->map, data->data[i], NULL);
    }

    return data;
}

static void *bench_hashmap_lookup(void *raw_data) {
    hashmap_data *data = raw_data;

    for(size_t i = 0; i < data->count; i++) {
        passgen_hashmap_lookup(&data->map, data->data[i]);
    }

    return data;
}

static void *hashmap_prepare_insert(const passgen_hashmap *opts) {
    hashmap_data *data = hashmap_prepare(opts);
    bench_hashmap_insert(data);
    return data;
}

const bench hashmap_insert = {
    .name = "hashmap_insert",
    .desc = "Insert data into hashmap",
    .prepare = &hashmap_prepare,
    .iterate = &bench_hashmap_insert,
    .cleanup = &hashmap_cleanup,
    .release = NULL,
    .multiplier = &hashmap_mult,
    .consumes = true,
    .unit = "items",
};

const bench hashmap_lookup = {
    .name = "hashmap_lookup",
    .desc = "Lookup data in hashmap",
    .prepare = &hashmap_prepare_insert,
    .iterate = &bench_hashmap_lookup,
    .cleanup = &hashmap_cleanup,
    .release = NULL,
    .multiplier = &hashmap_mult,
    .consumes = true,
    .unit = "items",
};
