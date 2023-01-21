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

void *stack_push_iterate(void *data) {
    void *stack = malloc(sizeof(passgen_stack));
    passgen_stack_init(stack, sizeof(payload));

    payload payload;
    payload.string = "This is a string";
    payload.decimals = 9.0;

    for(size_t i = 0; i < 100000; i++) {
        payload.integer = i;
        passgen_stack_push(stack, &payload);
    }

    return stack;
}

const bench stack_push = {
    .group = "stack",
    .name = "push",
    .desc = "Stack push benchmark",
    .prepare = NULL,
    .iterate = &stack_push_iterate,
    .cleanup = &passgen_stack_free,
    .release = NULL,
    .consumes = false,
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
