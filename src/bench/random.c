#include "bench.h"
#include <passgen/util/random.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define XORSHIFT_SEED 23456945749UL
#define UNUSED(x)     (void) x

struct bench_data {
    passgen_random random;
    uint64_t seed;
    size_t count;
    unsigned char *data;
    size_t max;
};

static void *bench_data_prepare_xorshift(const passgen_hashmap *opts) {
    UNUSED(opts);
    struct bench_data *data = malloc(sizeof(struct bench_data));
    data->count = 100000;
    data->max = 100000;
    data->seed = XORSHIFT_SEED;
    passgen_random_open_xorshift(&data->random, data->seed);
    return data;
}

static void *bench_data_prepare_system(const passgen_hashmap *opts) {
    UNUSED(opts);
    struct bench_data *data = malloc(sizeof(struct bench_data));
    data->count = 100000;
    data->max = 100000;
    passgen_random_open(&data->random, NULL);
    return data;
}

static void *bench_data_prepare_zero(const passgen_hashmap *opts) {
    UNUSED(opts);
    struct bench_data *data = malloc(sizeof(struct bench_data));
    data->count = 100000;
    data->max = 100000;
    passgen_random_open_zero(&data->random);
    return data;
}

static void *bench_data_prepare_xorshift_data(const passgen_hashmap *opts) {
    UNUSED(opts);
    struct bench_data *data = malloc(sizeof(struct bench_data));
    data->count = 100000;
    data->seed = XORSHIFT_SEED;
    data->data = malloc(data->count);
    passgen_random_open_xorshift(&data->random, data->seed);
    return data;
}

static void *bench_data_prepare_system_data(const passgen_hashmap *opts) {
    UNUSED(opts);
    struct bench_data *data = malloc(sizeof(struct bench_data));
    data->count = 100000;
    data->data = malloc(data->count);
    passgen_random_open(&data->random, NULL);
    return data;
}

static void *bench_data_prepare_zero_data(const passgen_hashmap *opts) {
    UNUSED(opts);
    struct bench_data *data = malloc(sizeof(struct bench_data));
    data->count = 100000;
    data->data = malloc(data->count);
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

static void *bench_random_read(void *raw_data) {
    struct bench_data *data = raw_data;
    passgen_random_read(&data->random, data->data, data->count);
    return NULL;
}

static void bench_data_release(void *raw_data) {
    struct bench_data *data = raw_data;
    passgen_random_close(&data->random);
    free(data);
}

static double bench_mult_u8(void *raw_data) {
    struct bench_data *data = raw_data;
    return data->count;
}

static double bench_mult_u16(void *raw_data) {
    struct bench_data *data = raw_data;
    return data->count * 2;
}

static double bench_mult_u32(void *raw_data) {
    struct bench_data *data = raw_data;
    return data->count * 4;
}

static double bench_mult_u64(void *raw_data) {
    struct bench_data *data = raw_data;
    return data->count * 8;
}

const bench random_xorshift_u8 = {
    .group = "random",
    .name = "xorshift_u8",
    .desc = "Stack push benchmark",
    .prepare = &bench_data_prepare_xorshift,
    .iterate = &bench_random_u8,
    .cleanup = NULL,
    .release = &bench_data_release,
    .multiplier = &bench_mult_u8,
    .consumes = false,
    .unit = "B",
};

const bench random_xorshift_u16 = {
    .group = "random",
    .name = "xorshift_u16",
    .desc = "Stack push benchmark",
    .prepare = &bench_data_prepare_xorshift,
    .iterate = &bench_random_u16,
    .cleanup = NULL,
    .release = &bench_data_release,
    .multiplier = &bench_mult_u16,
    .consumes = false,
    .unit = "B",
};

const bench random_xorshift_u32 = {
    .group = "random",
    .name = "xorshift_u32",
    .desc = "Stack push benchmark",
    .prepare = &bench_data_prepare_xorshift,
    .iterate = &bench_random_u32,
    .cleanup = NULL,
    .release = &bench_data_release,
    .multiplier = &bench_mult_u32,
    .consumes = false,
    .unit = "B",
};

const bench random_xorshift_u64 = {
    .group = "random",
    .name = "xorshift_u64",
    .desc = "Stack push benchmark",
    .prepare = &bench_data_prepare_xorshift,
    .iterate = &bench_random_u64,
    .cleanup = NULL,
    .release = &bench_data_release,
    .multiplier = &bench_mult_u64,
    .consumes = false,
    .unit = "B",
};

const bench random_xorshift_read = {
    .group = "random",
    .name = "xorshift_read",
    .desc = "Stack push benchmark",
    .prepare = &bench_data_prepare_xorshift_data,
    .iterate = &bench_random_read,
    .cleanup = NULL,
    .release = &bench_data_release,
    .multiplier = &bench_mult_u8,
    .consumes = false,
    .unit = "B",
};

const bench random_system_u8 = {
    .group = "random",
    .name = "system_u8",
    .desc = "Stack push benchmark",
    .prepare = &bench_data_prepare_system,
    .iterate = &bench_random_u8,
    .cleanup = NULL,
    .release = &bench_data_release,
    .multiplier = &bench_mult_u8,
    .consumes = false,
    .unit = "B",
};

const bench random_system_u16 = {
    .group = "random",
    .name = "system_u16",
    .desc = "Stack push benchmark",
    .prepare = &bench_data_prepare_system,
    .iterate = &bench_random_u16,
    .cleanup = NULL,
    .release = &bench_data_release,
    .multiplier = &bench_mult_u16,
    .consumes = false,
    .unit = "B",
};

const bench random_system_u32 = {
    .group = "random",
    .name = "system_u32",
    .desc = "Stack push benchmark",
    .prepare = &bench_data_prepare_system,
    .iterate = &bench_random_u32,
    .cleanup = NULL,
    .release = &bench_data_release,
    .multiplier = &bench_mult_u32,
    .consumes = false,
    .unit = "B",
};

const bench random_system_u64 = {
    .group = "random",
    .name = "system_u64",
    .desc = "Stack push benchmark",
    .prepare = &bench_data_prepare_system,
    .iterate = &bench_random_u64,
    .cleanup = NULL,
    .release = &bench_data_release,
    .multiplier = &bench_mult_u64,
    .consumes = false,
    .unit = "B",
};

const bench random_system_read = {
    .group = "random",
    .name = "system_read",
    .desc = "Stack push benchmark",
    .prepare = &bench_data_prepare_system_data,
    .iterate = &bench_random_read,
    .cleanup = NULL,
    .release = &bench_data_release,
    .multiplier = &bench_mult_u8,
    .consumes = false,
    .unit = "B",
};

const bench random_zero_u8 = {
    .group = "random",
    .name = "zero_u8",
    .desc = "Stack push benchmark",
    .prepare = &bench_data_prepare_zero,
    .iterate = &bench_random_u8,
    .cleanup = NULL,
    .release = &bench_data_release,
    .multiplier = &bench_mult_u8,
    .consumes = false,
    .unit = "B",
};

const bench random_zero_u16 = {
    .group = "random",
    .name = "zero_u16",
    .desc = "Stack push benchmark",
    .prepare = &bench_data_prepare_zero,
    .iterate = &bench_random_u16,
    .cleanup = NULL,
    .release = &bench_data_release,
    .multiplier = &bench_mult_u16,
    .consumes = false,
    .unit = "B",
};

const bench random_zero_u32 = {
    .group = "random",
    .name = "zero_u32",
    .desc = "Stack push benchmark",
    .prepare = &bench_data_prepare_zero,
    .iterate = &bench_random_u32,
    .cleanup = NULL,
    .release = &bench_data_release,
    .multiplier = &bench_mult_u32,
    .consumes = false,
    .unit = "B",
};

const bench random_zero_u64 = {
    .group = "random",
    .name = "zero_u64",
    .desc = "Stack push benchmark",
    .prepare = &bench_data_prepare_zero,
    .iterate = &bench_random_u64,
    .cleanup = NULL,
    .release = &bench_data_release,
    .multiplier = &bench_mult_u64,
    .consumes = false,
    .unit = "B",
};

const bench random_zero_read = {
    .group = "random",
    .name = "zero_read",
    .desc = "Stack push benchmark",
    .prepare = &bench_data_prepare_zero_data,
    .iterate = &bench_random_read,
    .cleanup = NULL,
    .release = &bench_data_release,
    .multiplier = &bench_mult_u8,
    .consumes = false,
    .unit = "B",
};
