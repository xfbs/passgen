#include <passgen/assert.h>
#include <passgen/config.h>
#include <passgen/util/array.h>
#include <stdlib.h>
#include <string.h>

#define SIZE_INITIAL 4

void passgen_array_realloc(passgen_array_t *array, size_t capacity) {
    array->data = realloc(array->data, capacity * sizeof(void *));
    array->capacity = capacity;
}

// Initialize array. Optionally takes a capacity, if so, memory that is
// sufficiently large to hold `capacity` items will be allocated.
void passgen_array_init(passgen_array_t *array, size_t capacity) {
    memset(array, 0, sizeof(*array));

    if(capacity) {
        passgen_array_realloc(array, capacity);
    }
}

// Push an item to the back of the array.
void passgen_array_push(passgen_array_t *array, void *pointer) {
    if(!array->data) {
        passgen_array_realloc(array, SIZE_INITIAL);
    }

    if(array->items == array->capacity) {
        passgen_array_realloc(array, array->capacity * 2);
    }

    array->data[array->items] = pointer;
    array->items += 1;
}

// Retrieve an item from the array.
void *passgen_array_get(passgen_array_t *array, size_t pos) {
#ifdef PASSGEN_DEBUG
    passgen_assert(pos <= array->items);
#endif

    return array->data[pos];
}

// Frees an array.
void passgen_array_free(passgen_array_t *array) {
    free(array->data);
    memset(array, 0, sizeof(*array));
}

// Pops the last item off the array and returns it.
void *passgen_array_pop(passgen_array_t *array) {
    if(!array->items) {
        return NULL;
    }

    array->items -= 1;
    return array->data[array->items];
}
