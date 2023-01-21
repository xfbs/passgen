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

void *stack_push_prepare(const passgen_hashmap *opts) {
    struct stack_bench_data *data = malloc(sizeof(struct stack_bench_data));
    data->count = 100000;
    passgen_stack_init(&data->stack, sizeof(payload));

    passgen_hashmap_entry *entry = passgen_hashmap_lookup(opts, "count");
    if(entry) {
        data->count = atoi(entry->value);
    }

    return data;
}

void stack_push_release(void *raw_data) {
    struct stack_bench_data *data = raw_data;
    passgen_stack_free(&data->stack);
    free(data);
}

void *stack_push_iterate(void *raw_data) {
    struct stack_bench_data *data = raw_data;

    payload payload;
    payload.string = "This is a string";
    payload.decimals = 9.0;

    for(size_t i = 0; i < data->count; i++) {
        payload.integer = i;
        passgen_stack_push(&data->stack, &payload);
    }

    return NULL;
}

const bench stack_push = {
    .group = "stack",
    .name = "push",
    .desc = "Stack push benchmark",
    .prepare = &stack_push_prepare,
    .iterate = &stack_push_iterate,
    .cleanup = NULL,
    .release = &stack_push_release,
    .consumes = true,
};

int bench_push_pop(int argc, char *argv[]) {
    if(argc != 3) {
        printf("Error: missing COUNT\n");
        return EXIT_FAILURE;
    }

    size_t insertions = atoi(argv[2]);

    passgen_stack stack;
    passgen_stack_init(&stack, sizeof(payload));

    payload payload;
    payload.string = "This is a string";
    payload.decimals = 9.0;

    for(size_t i = 0; i < insertions; i++) {
        payload.integer = i;
        passgen_stack_push(&stack, &payload);
    }

    for(size_t i = 0; i < insertions; i++) {
        passgen_stack_pop(&stack, &payload);
    }

    passgen_stack_free(&stack);

    printf("OK\n");

    return EXIT_SUCCESS;
}

int bench_small(int argc, char *argv[]) {
    return EXIT_SUCCESS;
}
