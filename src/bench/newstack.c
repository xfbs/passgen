#include <passgen/util/stack.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "newbench.h"

typedef struct payload {
    size_t integer;
    double decimals;
    const char *string;
} payload;

struct stack_bench_data {
    passgen_stack stack;
    size_t count;
};

static void *stack_push_prepare(const passgen_hashmap *opts) {
    struct stack_bench_data *data = malloc(sizeof(struct stack_bench_data));
    data->count = 100000;
    passgen_stack_init(&data->stack, sizeof(payload));

    passgen_hashmap_entry *entry = passgen_hashmap_lookup(opts, "count");
    if(entry) {
        data->count = atoi(entry->value);
    }

    return data;
}

static void *stack_pop_prepare(const passgen_hashmap *opts) {
    struct stack_bench_data *data = malloc(sizeof(struct stack_bench_data));
    data->count = 100000;
    passgen_stack_init(&data->stack, sizeof(payload));

    passgen_hashmap_entry *entry = passgen_hashmap_lookup(opts, "count");
    if(entry) {
        data->count = atoi(entry->value);
    }

    payload payload;
    payload.string = "This is a string";
    payload.decimals = 9.0;

    for(size_t i = 0; i < data->count; i++) {
        payload.integer = i;
        passgen_stack_push(&data->stack, &payload);
    }

    return data;
}

static void stack_push_release(void *raw_data) {
    struct stack_bench_data *data = raw_data;
    passgen_stack_free(&data->stack);
    free(data);
}

static void *stack_push_iterate(void *raw_data) {
    struct stack_bench_data *data = raw_data;

    payload payload;
    payload.string = "This is a string";
    payload.decimals = 9.0;

    for(size_t i = 0; i < data->count; i++) {
        payload.integer = i;
        passgen_stack_push(&data->stack, &payload);
    }

    return data;
}

static void *stack_pop_iterate(void *raw_data) {
    struct stack_bench_data *data = raw_data;

    for(size_t i = 0; i < data->count; i++) {
        payload payload;
        passgen_stack_pop(&data->stack, &payload);
    }

    return data;
}

const bench stack_push = {
    .group = "stack",
    .name = "push",
    .desc = "Stack push benchmark",
    .prepare = &stack_push_prepare,
    .iterate = &stack_push_iterate,
    .cleanup = &stack_push_release,
    .release = NULL,
    .consumes = true,
};

const bench stack_pop = {
    .group = "stack",
    .name = "pop",
    .desc = "Stack pop benchmark",
    .prepare = &stack_pop_prepare,
    .iterate = &stack_pop_iterate,
    .cleanup = &stack_push_release,
    .release = NULL,
    .consumes = true,
};
