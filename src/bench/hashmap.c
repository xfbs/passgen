#include "newbench.h"
#include <passgen/util/hashmap.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct bench_data {
    passgen_hashmap map;
    char **data;
    size_t count;
};

static double bench_mult(void *raw_data) {
    struct bench_data *data = raw_data;
    return data->count;
}

static void *data_prepare(const passgen_hashmap *opts) {
    (void) opts;
    struct bench_data *data = malloc(sizeof(struct bench_data));
    data->count = 50000;
    data->data = calloc(sizeof(const char *), data->count);
    for(size_t i = 0; i < data->count; i++) {
        data->data[i] = malloc(16);
        sprintf(data->data[i], "item-%zu", i);
    }
    passgen_hashmap_init(&data->map, &passgen_hashmap_context_default);
    return data;
}

static void data_cleanup(void *raw_data) {
    struct bench_data *data = raw_data;
    passgen_hashmap_free(&data->map);

    for(size_t i = 0; i < data->count; i++) {
        free(data->data[i]);
    }

    free(data);
}

static void *insert(void *raw_data) {
    struct bench_data *data = raw_data;

    for(size_t i = 0; i < data->count; i++) {
        passgen_hashmap_insert(&data->map, data->data[i], NULL);
    }

    return data;
}

const bench hashmap_insert = {
    .group = "hashmap",
    .name = "insert",
    .desc = "Insert data into a hashmap",
    .prepare = &data_prepare,
    .iterate = &insert,
    .cleanup = &data_cleanup,
    .release = NULL,
    .multiplier = &bench_mult,
    .consumes = true,
    .unit = "items",
};
