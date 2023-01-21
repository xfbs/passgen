#include <passgen/util/random.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "newbench.h"

#define XORSHIFT_SEED 23456945749UL

struct bench_data {
    passgen_random random;
    uint64_t seed;
    size_t count;
    size_t max;
};

static void *bench_data_prepare_xorshift(const passgen_hashmap *opts) {
    struct bench_data *data = malloc(sizeof(struct bench_data));
    data->count = 100000;
    data->max = 100000;
    data->seed = XORSHIFT_SEED;
    passgen_random_open_xorshift(&data->random, data->seed);
    return data;
}

static void *bench_data_prepare_system(const passgen_hashmap *opts) {
    struct bench_data *data = malloc(sizeof(struct bench_data));
    data->count = 100000;
    data->max = 100000;
    data->seed = XORSHIFT_SEED;
    passgen_random_open(&data->random);
    return data;
}

static void *bench_data_prepare_zero(const passgen_hashmap *opts) {
    struct bench_data *data = malloc(sizeof(struct bench_data));
    data->count = 100000;
    data->max = 100000;
    data->seed = XORSHIFT_SEED;
    passgen_random_open_zero(&data->random);
    return data;
}

static void *bench_random_u8(void *raw_data) {
    struct bench_data *data = raw_data;
    for(size_t i = 0; i < data->count; i++) {
        passgen_random_u8(&data->random);
    }

    return NULL;
}

static void *bench_random_u16(void *raw_data) {
    struct bench_data *data = raw_data;
    for(size_t i = 0; i < data->count; i++) {
        passgen_random_u16(&data->random);
    }

    return NULL;
}

static void *bench_random_u32(void *raw_data) {
    struct bench_data *data = raw_data;
    for(size_t i = 0; i < data->count; i++) {
        passgen_random_u32(&data->random);
    }

    return NULL;
}

static void *bench_random_u64(void *raw_data) {
    struct bench_data *data = raw_data;
    for(size_t i = 0; i < data->count; i++) {
        passgen_random_u64(&data->random);
    }

    return NULL;
}

static void bench_data_release(void *raw_data) {
    struct bench_data *data = raw_data;
    passgen_random_close(&data->random);
    free(data);
}

const bench random_xorshift_u8 = {
    .group = "random",
    .name = "xorshift_u8",
    .desc = "Stack push benchmark",
    .prepare = &bench_data_prepare_xorshift,
    .iterate = &bench_random_u8,
    .cleanup = NULL,
    .release = &bench_data_release,
    .consumes = false,
};

const bench random_xorshift_u16 = {
    .group = "random",
    .name = "xorshift_u16",
    .desc = "Stack push benchmark",
    .prepare = &bench_data_prepare_xorshift,
    .iterate = &bench_random_u16,
    .cleanup = NULL,
    .release = &bench_data_release,
    .consumes = false,
};

const bench random_xorshift_u32 = {
    .group = "random",
    .name = "xorshift_u32",
    .desc = "Stack push benchmark",
    .prepare = &bench_data_prepare_xorshift,
    .iterate = &bench_random_u32,
    .cleanup = NULL,
    .release = &bench_data_release,
    .consumes = false,
};

const bench random_xorshift_u64 = {
    .group = "random",
    .name = "xorshift_u64",
    .desc = "Stack push benchmark",
    .prepare = &bench_data_prepare_xorshift,
    .iterate = &bench_random_u64,
    .cleanup = NULL,
    .release = &bench_data_release,
    .consumes = false,
};


const bench random_system_u8 = {
    .group = "random",
    .name = "system_u8",
    .desc = "Stack push benchmark",
    .prepare = &bench_data_prepare_system,
    .iterate = &bench_random_u8,
    .cleanup = NULL,
    .release = &bench_data_release,
    .consumes = false,
};

const bench random_system_u16 = {
    .group = "random",
    .name = "system_u16",
    .desc = "Stack push benchmark",
    .prepare = &bench_data_prepare_system,
    .iterate = &bench_random_u16,
    .cleanup = NULL,
    .release = &bench_data_release,
    .consumes = false,
};

const bench random_system_u32 = {
    .group = "random",
    .name = "system_u32",
    .desc = "Stack push benchmark",
    .prepare = &bench_data_prepare_system,
    .iterate = &bench_random_u32,
    .cleanup = NULL,
    .release = &bench_data_release,
    .consumes = false,
};

const bench random_system_u64 = {
    .group = "random",
    .name = "system_u64",
    .desc = "Stack push benchmark",
    .prepare = &bench_data_prepare_system,
    .iterate = &bench_random_u64,
    .cleanup = NULL,
    .release = &bench_data_release,
    .consumes = false,
};


const bench random_zero_u8 = {
    .group = "random",
    .name = "zero_u8",
    .desc = "Stack push benchmark",
    .prepare = &bench_data_prepare_zero,
    .iterate = &bench_random_u8,
    .cleanup = NULL,
    .release = &bench_data_release,
    .consumes = false,
};

const bench random_zero_u16 = {
    .group = "random",
    .name = "zero_u16",
    .desc = "Stack push benchmark",
    .prepare = &bench_data_prepare_zero,
    .iterate = &bench_random_u16,
    .cleanup = NULL,
    .release = &bench_data_release,
    .consumes = false,
};

const bench random_zero_u32 = {
    .group = "random",
    .name = "zero_u32",
    .desc = "Stack push benchmark",
    .prepare = &bench_data_prepare_zero,
    .iterate = &bench_random_u32,
    .cleanup = NULL,
    .release = &bench_data_release,
    .consumes = false,
};

const bench random_zero_u64 = {
    .group = "random",
    .name = "zero_u64",
    .desc = "Stack push benchmark",
    .prepare = &bench_data_prepare_zero,
    .iterate = &bench_random_u64,
    .cleanup = NULL,
    .release = &bench_data_release,
    .consumes = false,
};

