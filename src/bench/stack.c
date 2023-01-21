#include <passgen/util/stack.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct bencher {
} bencher;

typedef struct bench_arg {
    const char *name;
    const char *desc;
    const char *value;
} bench_arg;

/// Benchmark definition
typedef struct bench {
    const char *group;
    const char *name;
    const char *description;
    bench_arg *args;
    bool consumes;
    void *(*prepare)(const char *args[]);
    void *(*iterate)(void *data);
    void (*cleanup)(void *data);
    void (*release)(void *data);
} bench;

void bench_add(
    bencher *bencher,
    bench bench);

typedef struct payload {
    size_t integer;
    double decimals;
    const char *string;
} payload;

bench push_pop = (bench) {
    .group = "stack",
    .name = "push_pop",
    .description = "Push and pop COUNT elements",
    .args = &(bench_arg[]) {
        {
            .name = "count",
            .desc = "Elements to push and pop",
            .value = "1000000",
        },
        {
            .name = NULL,
        }
    },
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

int main(int argc, char *argv[]) {
    if(argc < 2) {
        printf("passgen-stack-bench: Benchmarks passgen_stack insertions\n");
        printf("Usage: %s BENCHMARK COUNT\n\n", argv[0]);
        printf("BENCHMARKS\n");
        printf("    push_pop  Push and pop COUNT items into a passgen_stack\n");
        printf("    small     Create COUNT small passgen_stacks\n");
        return EXIT_FAILURE;
    }

    const char *benchmark = argv[1];

    if(0 == strcmp(benchmark, "push_pop")) {
        return bench_push_pop(argc, argv);
    } else if(0 == strcmp(benchmark, "small")) {
        return bench_small(argc, argv);
    }
}
