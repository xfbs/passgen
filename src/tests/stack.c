#include "tests.h"
#include "passgen/container/stack_new.h"

test_result test_stack_init(void) {
    passgen_stack stack;
    passgen_stack_init(&stack, sizeof(size_t));
    passgen_stack_free(&stack);
    return test_ok;
}

test_result test_stack_push_one(void) {
    passgen_stack stack;
    passgen_stack_init(&stack, sizeof(size_t));

    size_t value = 2;
    size_t *value_ptr = passgen_stack_push(&stack, &value);
    assert(value_ptr);
    assert_eq(value, *value_ptr);

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

    value = 2;
    value_ptr = passgen_stack_push(&stack, &value);
    assert(value_ptr);
    assert_eq(value, *value_ptr);

    value = 3;
    value_ptr = passgen_stack_push(&stack, &value);
    assert(value_ptr);
    assert_eq(value, *value_ptr);

    passgen_stack_free(&stack);
    return test_ok;
}

test_result test_stack_push_bin_size(void) {
    passgen_stack stack;
    passgen_stack_init(&stack, sizeof(size_t));

    for(size_t i = 0; i < stack.bin_size; i++) {
        size_t *value_ptr = passgen_stack_push(&stack, &i);
        assert(value_ptr);
        assert_eq(i, *value_ptr);
    }

    passgen_stack_free(&stack);
    return test_ok;
}
