#include "bench.h"
#include <passgen/random.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define XORSHIFT_SEED 23456945749UL
#define UNUSED(x)     (void) x

struct bench_data {
    passgen_random random;
    size_t count;
    unsigned char *data;
    size_t max;
};

static void *bench_data_prepare(const passgen_hashmap *opts) {
    UNUSED(opts);
    struct bench_data *data = malloc(sizeof(struct bench_data));
    data->count = 100000;
    data->max = 100000;
    data->data = NULL;
    return data;
}

static void *bench_data_prepare_xorshift(const passgen_hashmap *opts) {
    struct bench_data *data = bench_data_prepare(opts);
    passgen_random_xorshift_open(&data->random, XORSHIFT_SEED);
    return data;
}

static void *bench_data_prepare_system(const passgen_hashmap *opts) {
    struct bench_data *data = bench_data_prepare(opts);
    passgen_random_open(&data->random, NULL);
    return data;
}

static void *bench_data_prepare_zero(const passgen_hashmap *opts) {
    struct bench_data *data = bench_data_prepare(opts);
    passgen_random_zero_open(&data->random);
    return data;
}

static void bench_prepare_data(struct bench_data *data) {
    data->data = malloc(data->count);
}

static void *bench_data_prepare_xorshift_data(const passgen_hashmap *opts) {
    struct bench_data *data = bench_data_prepare_xorshift(opts);
    bench_prepare_data(data);
    return data;
}

static void *bench_data_prepare_system_data(const passgen_hashmap *opts) {
    struct bench_data *data = bench_data_prepare_system(opts);
    bench_prepare_data(data);
    return data;
}

static void *bench_data_prepare_zero_data(const passgen_hashmap *opts) {
    struct bench_data *data = bench_data_prepare_zero(opts);
    bench_prepare_data(data);
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
    if(data->data) {
        free(data->data);
    }
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
    .name = "random_xorshift_u8",
    .desc = "Stack push benchmark",
    .prepare = &bench_data_prepare_xorshift,
    .iterate = &bench_random_u8,
    .cleanup = NULL,
    .release = &bench_data_release,
    .multiplier = &bench_mult_u8,
    .consumes = false,
    .unit = "bytes",
};

const bench random_xorshift_u16 = {
    .name = "random_xorshift_u16",
    .desc = "Stack push benchmark",
    .prepare = &bench_data_prepare_xorshift,
    .iterate = &bench_random_u16,
    .cleanup = NULL,
    .release = &bench_data_release,
    .multiplier = &bench_mult_u16,
    .consumes = false,
    .unit = "bytes",
};

const bench random_xorshift_u32 = {
    .name = "random_xorshift_u32",
    .desc = "Stack push benchmark",
    .prepare = &bench_data_prepare_xorshift,
    .iterate = &bench_random_u32,
    .cleanup = NULL,
    .release = &bench_data_release,
    .multiplier = &bench_mult_u32,
    .consumes = false,
    .unit = "bytes",
};

const bench random_xorshift_u64 = {
    .name = "random_xorshift_u64",
    .desc = "Stack push benchmark",
    .prepare = &bench_data_prepare_xorshift,
    .iterate = &bench_random_u64,
    .cleanup = NULL,
    .release = &bench_data_release,
    .multiplier = &bench_mult_u64,
    .consumes = false,
    .unit = "bytes",
};

const bench random_xorshift_read = {
    .name = "random_xorshift_read",
    .desc = "Stack push benchmark",
    .prepare = &bench_data_prepare_xorshift_data,
    .iterate = &bench_random_read,
    .cleanup = NULL,
    .release = &bench_data_release,
    .multiplier = &bench_mult_u8,
    .consumes = false,
    .unit = "bytes",
};

const bench random_system_u8 = {
    .name = "random_system_u8",
    .desc = "Stack push benchmark",
    .prepare = &bench_data_prepare_system,
    .iterate = &bench_random_u8,
    .cleanup = NULL,
    .release = &bench_data_release,
    .multiplier = &bench_mult_u8,
    .consumes = false,
    .unit = "bytes",
};

const bench random_system_u16 = {
    .name = "random_system_u16",
    .desc = "Stack push benchmark",
    .prepare = &bench_data_prepare_system,
    .iterate = &bench_random_u16,
    .cleanup = NULL,
    .release = &bench_data_release,
    .multiplier = &bench_mult_u16,
    .consumes = false,
    .unit = "bytes",
};

const bench random_system_u32 = {
    .name = "random_system_u32",
    .desc = "Stack push benchmark",
    .prepare = &bench_data_prepare_system,
    .iterate = &bench_random_u32,
    .cleanup = NULL,
    .release = &bench_data_release,
    .multiplier = &bench_mult_u32,
    .consumes = false,
    .unit = "bytes",
};

const bench random_system_u64 = {
    .name = "random_system_u64",
    .desc = "Stack push benchmark",
    .prepare = &bench_data_prepare_system,
    .iterate = &bench_random_u64,
    .cleanup = NULL,
    .release = &bench_data_release,
    .multiplier = &bench_mult_u64,
    .consumes = false,
    .unit = "bytes",
};

const bench random_system_read = {
    .name = "random_system_read",
    .desc = "Stack push benchmark",
    .prepare = &bench_data_prepare_system_data,
    .iterate = &bench_random_read,
    .cleanup = NULL,
    .release = &bench_data_release,
    .multiplier = &bench_mult_u8,
    .consumes = false,
    .unit = "bytes",
};

const bench random_zero_u8 = {
    .name = "random_zero_u8",
    .desc = "Stack push benchmark",
    .prepare = &bench_data_prepare_zero,
    .iterate = &bench_random_u8,
    .cleanup = NULL,
    .release = &bench_data_release,
    .multiplier = &bench_mult_u8,
    .consumes = false,
    .unit = "bytes",
};

const bench random_zero_u16 = {
    .name = "random_zero_u16",
    .desc = "Stack push benchmark",
    .prepare = &bench_data_prepare_zero,
    .iterate = &bench_random_u16,
    .cleanup = NULL,
    .release = &bench_data_release,
    .multiplier = &bench_mult_u16,
    .consumes = false,
    .unit = "bytes",
};

const bench random_zero_u32 = {
    .name = "random_zero_u32",
    .desc = "Stack push benchmark",
    .prepare = &bench_data_prepare_zero,
    .iterate = &bench_random_u32,
    .cleanup = NULL,
    .release = &bench_data_release,
    .multiplier = &bench_mult_u32,
    .consumes = false,
    .unit = "bytes",
};

const bench random_zero_u64 = {
    .name = "random_zero_u64",
    .desc = "Stack push benchmark",
    .prepare = &bench_data_prepare_zero,
    .iterate = &bench_random_u64,
    .cleanup = NULL,
    .release = &bench_data_release,
    .multiplier = &bench_mult_u64,
    .consumes = false,
    .unit = "bytes",
};

const bench random_zero_read = {
    .name = "random_zero_read",
    .desc = "Stack push benchmark",
    .prepare = &bench_data_prepare_zero_data,
    .iterate = &bench_random_read,
    .cleanup = NULL,
    .release = &bench_data_release,
    .multiplier = &bench_mult_u8,
    .consumes = false,
    .unit = "bytes",
};
