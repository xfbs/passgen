#include "passgen/data/array.h"
#include "tests.h"
#include <string.h>

test_result test_array_init(void) {
    passgen_array_t array;
    passgen_array_init(&array, 256, NULL);

    assert(array.data == NULL);
    assert(array.len == 0);
    assert(array.bins == 0);

    passgen_array_free(&array, 256, NULL);

    return test_ok;
}

test_result test_array_push(void) {
    passgen_array_t array;
    passgen_array_init(&array, 256, NULL);

    void *data;

    data = passgen_array_push(&array, 256, NULL);
    assert(data);
    assert(data == (array.data[0] + 0));
    assert(array.len == 1);
    assert(array.bins == 4);
    memset(data, 0, 256);

    data = passgen_array_push(&array, 256, NULL);
    assert(data);
    assert(data == (array.data[0] + 256));
    assert(array.len == 2);
    assert(array.bins == 4);
    memset(data, 0, 256);

    data = passgen_array_push(&array, 256, NULL);
    assert(data);
    assert(data == (array.data[0] + 512));
    assert(array.len == 3);
    assert(array.bins == 4);
    memset(data, 0, 256);

    data = passgen_array_push(&array, 256, NULL);
    assert(data);
    assert(data == (array.data[0] + 768));
    assert(array.len == 4);
    assert(array.bins == 4);
    memset(data, 0, 256);

    data = passgen_array_push(&array, 256, NULL);
    assert(data);
    assert(data == (array.data[1] + 0));
    assert(array.len == 5);
    assert(array.bins == 4);
    memset(data, 0, 256);

    for(size_t i = 0; i < 11; i++) {
        data = passgen_array_push(&array, 256, NULL);
        assert(data);
        assert(array.len == (6 + i));
        assert(array.bins == 4);
        memset(data, 0, 256);
    }

    data = passgen_array_push(&array, 256, NULL);
    assert(data);
    assert(data == (array.data[4] + 0));
    assert(array.len == 17);
    assert(array.bins == 8);
    memset(data, 0, 256);

    passgen_array_free(&array, 256, NULL);

    return test_ok;
}

test_result test_array_get(void) {
    passgen_array_t array;
    passgen_array_init(&array, 256, NULL);

    void *data;

    data = passgen_array_push(&array, 256, NULL);
    assert(data);
    assert(data == (array.data[0] + 0));
    assert(array.len == 1);
    assert(array.bins == 4);
    memset(data, 0, 256);

    data = passgen_array_push(&array, 256, NULL);
    assert(data);
    assert(data == (array.data[0] + 256));
    assert(array.len == 2);
    assert(array.bins == 4);
    memset(data, 0, 256);

    data = passgen_array_push(&array, 256, NULL);
    assert(data);
    assert(data == (array.data[0] + 512));
    assert(array.len == 3);
    assert(array.bins == 4);
    memset(data, 0, 256);

    data = passgen_array_push(&array, 256, NULL);
    assert(data);
    assert(data == (array.data[0] + 768));
    assert(array.len == 4);
    assert(array.bins == 4);
    memset(data, 0, 256);

    data = passgen_array_push(&array, 256, NULL);
    assert(data);
    assert(data == (array.data[1] + 0));
    assert(array.len == 5);
    assert(array.bins == 4);
    memset(data, 0, 256);

    // no bounds checking
    assert(passgen_array_get(&array, 256, 0) == (array.data[0] + 0));
    assert(passgen_array_get(&array, 256, 1) == (array.data[0] + 256));
    assert(passgen_array_get(&array, 256, 2) == (array.data[0] + 512));
    assert(passgen_array_get(&array, 256, 3) == (array.data[0] + 768));
    assert(passgen_array_get(&array, 256, 4) == (array.data[1] + 0));

    // when compiled in debug mode, this fails due to a runtime check.
    // assert(passgen_array_get(&array, 256, 5) == (array.data[1] + 256));
    // assert(passgen_array_get(&array, 256, 6) == (array.data[1] + 512));
    // assert(passgen_array_get(&array, 256, 7) == (array.data[1] + 768));

    passgen_array_free(&array, 256, NULL);

    return test_ok;
}

test_result test_array_pop(void) {
    passgen_array_t array;
    passgen_array_init(&array, 256, NULL);

    void *data;

    data = passgen_array_push(&array, 256, NULL);
    assert(data);
    assert(data == (array.data[0] + 0));
    assert(array.len == 1);
    assert(array.bins == 4);
    memset(data, 0, 256);

    // removing last element frees the whole thing.
    passgen_array_pop(&array, 256, NULL);
    assert(array.len == 0);
    assert(array.bins == 0);
    assert(array.data == NULL);

    data = passgen_array_push(&array, 256, NULL);
    assert(data);
    assert(data == (array.data[0] + 0));
    assert(array.len == 1);
    assert(array.bins == 4);
    memset(data, 0, 256);

    data = passgen_array_push(&array, 256, NULL);
    assert(data);
    assert(data == (array.data[0] + 256));
    assert(array.len == 2);
    assert(array.bins == 4);
    memset(data, 0, 256);

    data = passgen_array_push(&array, 256, NULL);
    assert(data);
    assert(data == (array.data[0] + 512));
    assert(array.len == 3);
    assert(array.bins == 4);
    memset(data, 0, 256);

    data = passgen_array_push(&array, 256, NULL);
    assert(data);
    assert(data == (array.data[0] + 768));
    assert(array.len == 4);
    assert(array.bins == 4);
    memset(data, 0, 256);

    data = passgen_array_push(&array, 256, NULL);
    assert(data);
    assert(data == (array.data[1] + 0));
    assert(array.len == 5);
    assert(array.bins == 4);
    memset(data, 0, 256);

    data = passgen_array_push(&array, 256, NULL);
    assert(data);
    assert(data == (array.data[1] + 256));
    assert(array.len == 6);
    assert(array.bins == 4);
    memset(data, 0, 256);

    passgen_array_pop(&array, 256, NULL);
    assert(array.len == 5);
    assert(array.bins == 4);

    passgen_array_pop(&array, 256, NULL);
    assert(array.len == 4);
    assert(array.bins == 4);
    // assert(array.data[1] == NULL);

    passgen_array_get(&array, 256, 0);

    passgen_array_pop(&array, 256, NULL);
    assert(array.len == 3);

    passgen_array_pop(&array, 256, NULL);
    assert(array.len == 2);

    passgen_array_pop(&array, 256, NULL);
    assert(array.len == 1);

    passgen_array_free(&array, 256, NULL);

    return test_ok;
}
