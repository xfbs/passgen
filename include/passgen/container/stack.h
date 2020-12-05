#pragma once
#include <stddef.h>

// forward declaration, to remove dependency.
struct passgen_mem;
typedef struct passgen_mem passgen_mem_t;

#define PASSGEN_STACK_DECLARE(data_type, name)                              \
    struct name##_stack {                                                   \
        data_type **data;                                                   \
        size_t len;                                                         \
        size_t bins;                                                        \
    };                                                                      \
                                                                            \
    void name##_stack_init(struct name##_stack *stack, passgen_mem_t *mem); \
    void name##_stack_free(struct name##_stack *stack, passgen_mem_t *mem); \
                                                                            \
    data_type *name##_stack_push(                                           \
        struct name##_stack *stack,                                         \
        passgen_mem_t *mem);                                                \
                                                                            \
    data_type *name##_stack_get(struct name##_stack *stack, size_t pos);    \
    void name##_stack_pop(struct name##_stack *stack, passgen_mem_t *mem);  \
    data_type *name##_stack_top(struct name##_stack *stack)
