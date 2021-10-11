#include "tests.h"
#include "passgen/container/stack_new.h"
#include <stdlib.h>

test_result test_stack_init(void) {
    passgen_stack stack;
    passgen_stack_init(&stack, sizeof(size_t));
    passgen_stack_free(&stack);
    return test_ok;
}

test_result test_stack_push_one(void) {
    passgen_stack stack;
    passgen_stack_init(&stack, sizeof(size_t));
    assert_eq(stack.len, 0);

    size_t value = 2;
    size_t *value_ptr = passgen_stack_push(&stack, &value);
    assert(value_ptr);
    assert_eq(value, *value_ptr);
    assert_eq(stack.len, 1);

    passgen_stack_free(&stack);
    return test_ok;
}

test_result test_stack_push_multiple(void) {
    passgen_stack stack;
    passgen_stack_init(&stack, sizeof(size_t));

    size_t value;
    size_t *value_ptr;

    value = 1;
    value_ptr = passgen_stack_push(&stack, &value);
    assert(value_ptr);
    assert_eq(value, *value_ptr);
    assert_eq(stack.len, 1);

    value = 2;
    value_ptr = passgen_stack_push(&stack, NULL);
    assert(value_ptr);
    *value_ptr = value;
    assert_eq(value, *value_ptr);
    assert_eq(stack.len, 2);

    value = 3;
    value_ptr = passgen_stack_push(&stack, &value);
    assert(value_ptr);
    assert_eq(value, *value_ptr);
    assert_eq(stack.len, 3);

    passgen_stack_free(&stack);
    return test_ok;
}

test_result test_stack_push_bin_size(void) {
    passgen_stack stack;
    passgen_stack_init(&stack, sizeof(size_t));

    for(size_t i = 0; i < stack.bin_size; i++) {
        assert_eq(stack.len, i);
        size_t *value_ptr = passgen_stack_push(&stack, &i);
        assert(value_ptr);
        assert_eq(i, *value_ptr);
    }

    passgen_stack_free(&stack);
    return test_ok;
}

test_result test_stack_push_many(void) {
    passgen_stack stack;
    passgen_stack_init(&stack, sizeof(size_t));
    size_t len_max = 7 * stack.bin_size - 3;

    for(size_t i = 0; i < len_max; i++) {
        assert_eq(stack.len, i);
        size_t *value_ptr = passgen_stack_push(&stack, &i);
        assert(value_ptr);
        assert_eq(i, *value_ptr);
    }

    assert_eq(stack.len, len_max);
    passgen_stack_free(&stack);
    return test_ok;
}

test_result test_stack_get_one(void) {
    passgen_stack stack;
    passgen_stack_init(&stack, sizeof(size_t));
    size_t *value_ptr;
    size_t value = 1;

    value_ptr = passgen_stack_push(&stack, &value);
    assert(value_ptr);
    assert_eq(value, *value_ptr);

    value_ptr = passgen_stack_get(&stack, 0);
    assert(value_ptr);
    assert_eq(value, *value_ptr);

    passgen_stack_free(&stack);
    return test_ok;
}

test_result test_stack_get_two(void) {
    passgen_stack stack;
    passgen_stack_init(&stack, sizeof(size_t));
    size_t *value_ptr;
    size_t value = 1;

    value_ptr = passgen_stack_push(&stack, &value);
    assert(value_ptr);
    assert_eq(value, *value_ptr);

    value = 2;
    value_ptr = passgen_stack_push(&stack, &value);
    assert(value_ptr);
    assert_eq(value, *value_ptr);

    value_ptr = passgen_stack_get(&stack, 0);
    assert(value_ptr);
    assert_eq(1, *value_ptr);

    value_ptr = passgen_stack_get(&stack, 1);
    assert(value_ptr);
    assert_eq(2, *value_ptr);

    passgen_stack_free(&stack);
    return test_ok;
}

test_result test_stack_get_many(void) {
    passgen_stack stack;
    passgen_stack_init(&stack, sizeof(size_t));
    size_t max_len = 7 * stack.bin_size - 3;

    for(size_t i = 0; i < max_len; i++) {
        size_t *value_ptr = passgen_stack_push(&stack, &i);
        assert(value_ptr);
        assert_eq(i, *value_ptr);
    }

    for(size_t i = 0; i < stack.len; i++) {
        size_t *value_ptr = passgen_stack_get(&stack, i);
        assert(value_ptr);
        assert_eq(i, *value_ptr);
    }

    passgen_stack_free(&stack);
    return test_ok;
}

test_result test_stack_top_empty(void) {
    passgen_stack stack;
    passgen_stack_init(&stack, sizeof(size_t));

    assert(!passgen_stack_top(&stack));

    passgen_stack_free(&stack);
    return test_ok;
}

test_result test_stack_top_one(void) {
    passgen_stack stack;
    passgen_stack_init(&stack, sizeof(size_t));

    size_t value = 1;
    size_t *value_ptr;

    value_ptr = passgen_stack_push(&stack, &value);
    assert(value_ptr);
    assert_eq(value, *value_ptr);

    // should return the exact same pointer as push did
    assert_eq(value_ptr, passgen_stack_top(&stack));

    passgen_stack_free(&stack);
    return test_ok;
}

test_result test_stack_top_many(void) {
    passgen_stack stack;
    passgen_stack_init(&stack, sizeof(size_t));
    size_t len_max = 8 * stack.bin_size;

    for(size_t i = 0; i < len_max; i++) {
        size_t *value_ptr = passgen_stack_push(&stack, &i);
        assert(value_ptr);
        assert_eq(i, *value_ptr);
        assert_eq(value_ptr, passgen_stack_top(&stack));
    }

    passgen_stack_free(&stack);
    return test_ok;
}

void element_free(void *data) {
    void **real_data = data;
    free(*real_data);
}

test_result test_stack_foreach_empty(void) {
    passgen_stack stack;
    passgen_stack_init(&stack, sizeof(size_t));
    passgen_stack_foreach(&stack, element_free);
    passgen_stack_free(&stack);
    return test_ok;
}

test_result test_stack_foreach_one(void) {
    passgen_stack stack;
    passgen_stack_init(&stack, sizeof(void *));
    void *data = malloc(255);
    passgen_stack_push(&stack, &data);
    passgen_stack_foreach(&stack, element_free);
    passgen_stack_free(&stack);
    return test_ok;
}

test_result test_stack_foreach_many(void) {
    passgen_stack stack;
    passgen_stack_init(&stack, sizeof(void *));
    size_t max_len = 8 * stack.bin_size - 3;

    for(size_t i = 0; i < max_len; i++) {
        void *data = malloc(255);
        passgen_stack_push(&stack, &data);
    }

    passgen_stack_foreach(&stack, element_free);
    passgen_stack_free(&stack);
    return test_ok;
}

test_result test_stack_pop_empty(void) {
    passgen_stack stack;
    passgen_stack_init(&stack, sizeof(size_t));

    size_t value;
    assert_eq(passgen_stack_pop(&stack, &value), NULL);

    passgen_stack_free(&stack);
    return test_ok;
}

test_result test_stack_pop_one(void) {
    passgen_stack stack;
    passgen_stack_init(&stack, sizeof(size_t));

    size_t value = 1;
    passgen_stack_push(&stack, &value);

    size_t top = 0;
    assert_eq(passgen_stack_pop(&stack, &top), &top);
    assert_eq(top, value);

    passgen_stack_free(&stack);
    return test_ok;
}

test_result test_stack_pop_some(void) {
    passgen_stack stack;
    passgen_stack_init(&stack, sizeof(size_t));

    for(size_t i = 0; i < stack.bin_size; i++) {
        size_t *value = passgen_stack_push(&stack, &i);
        assert(value);
        assert_eq(*value, i);
    }

    for(size_t i = 0; i < stack.bin_size; i++) {
        size_t value;
        size_t *ret = passgen_stack_pop(&stack, &value);
        assert_eq(ret, &value);
        assert_eq(value, stack.len);
    }

    assert_eq(stack.len, 0);

    passgen_stack_free(&stack);
    return test_ok;
}

test_result test_stack_pop_many(void) {
    passgen_stack stack;
    passgen_stack_init(&stack, sizeof(size_t));
    size_t max_len = 7 * stack.bin_size - 3;

    for(size_t i = 0; i < max_len; i++) {
        size_t *value = passgen_stack_push(&stack, &i);
        assert(value);
        assert_eq(*value, i);
    }

    for(size_t i = 0; i < max_len; i++) {
        size_t value;
        size_t *ret = passgen_stack_pop(&stack, &value);
        assert_eq(ret, &value);
        assert_eq(value, stack.len);
    }

    assert_eq(stack.len, 0);

    passgen_stack_free(&stack);
    return test_ok;
}
