#include "passgen/util/array.h"
#include "tests.h"
#include <string.h>

test_result test_array_init(void) {
    size_t capacity = 15;
    passgen_array array;
    passgen_array_init(&array, capacity);
    assert_eq(array.capacity, capacity);
    assert_eq(array.items, 0);
    assert(array.data != NULL);

    passgen_array_free(&array);

    return test_ok;
}

test_result test_array_push(void) {
    size_t capacity = 15;
    passgen_array array;
    passgen_array_init(&array, capacity);
    assert_eq(array.capacity, capacity);
    assert_eq(array.items, 0);
    assert(array.data != NULL);

    int data1 = 7;
    passgen_array_push(&array, &data1);
    assert_eq(array.capacity, capacity);
    assert_eq(array.items, 1);
    assert_eq(array.data[0], &data1);

    int data2 = 8;
    passgen_array_push(&array, &data2);
    assert_eq(array.capacity, capacity);
    assert_eq(array.items, 2);
    assert_eq(array.data[1], &data2);

    passgen_array_free(&array);

    return test_ok;
}

test_result test_array_get(void) {
    size_t capacity = 15;
    passgen_array array;
    passgen_array_init(&array, capacity);
    assert_eq(array.capacity, capacity);
    assert_eq(array.items, 0);
    assert(array.data != NULL);

    int data1 = 7;
    int data2 = 7;
    int data3 = 7;
    passgen_array_push(&array, &data1);
    passgen_array_push(&array, &data2);
    passgen_array_push(&array, &data3);

    assert_eq(passgen_array_get(&array, 0), &data1);
    assert_eq(passgen_array_get(&array, 1), &data2);
    assert_eq(passgen_array_get(&array, 2), &data3);

    passgen_array_free(&array);

    return test_ok;
}

test_result test_array_pop(void) {
    size_t capacity = 15;
    passgen_array array;
    passgen_array_init(&array, capacity);
    assert_eq(array.capacity, capacity);
    assert_eq(array.items, 0);
    assert(array.data != NULL);

    int data1 = 7;
    int data2 = 7;
    int data3 = 7;
    passgen_array_push(&array, &data1);
    passgen_array_push(&array, &data2);
    passgen_array_push(&array, &data3);

    assert_eq(passgen_array_pop(&array), &data3);
    assert_eq(array.items, 2);

    assert_eq(passgen_array_pop(&array), &data2);
    assert_eq(array.items, 1);

    assert_eq(passgen_array_pop(&array), &data1);
    assert_eq(array.items, 0);

    assert_eq(passgen_array_pop(&array), NULL);

    passgen_array_free(&array);

    return test_ok;
}

test_result test_array_realloc(void) {
    size_t capacity = 15;
    passgen_array array;
    passgen_array_init(&array, capacity);
    assert_eq(array.capacity, capacity);
    assert_eq(array.items, 0);
    assert(array.data != NULL);

    int data = 7;
    passgen_array_push(&array, &data);
    assert_eq(array.capacity, capacity);
    assert_eq(array.items, 1);
    assert_eq(array.data[0], &data);

    size_t new_capacity = 1500;
    passgen_array_realloc(&array, new_capacity);
    assert_eq(array.capacity, new_capacity);
    assert_eq(array.items, 1);
    assert_eq(array.data[0], &data);

    passgen_array_free(&array);

    return test_ok;
}
