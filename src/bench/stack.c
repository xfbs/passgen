#include <passgen/util/stack.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct payload {
    size_t integer;
    double decimals;
    const char *string;
} payload;

int main(int argc, char *argv[]) {
    if(argc != 2) {
        printf("passgen-stack-bench: Benchmarks passgen_stack insertions\n");
        printf("Usage: %s INSERTIONS\n", argv[0]);
        return EXIT_FAILURE;
    }

    size_t insertions = atoi(argv[1]);

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

    return EXIT_SUCCESS;
}
