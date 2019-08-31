#include "passgen/array.h"
#include <assert.h>

#define BINS_INITIAL 4
#define BINS_MULTIPLIER 2
#define BIN_SIZE 1024
#define ITEMS_PER_BIN(size) (BIN_SIZE / size)

passgen_array_t passgen_array_init(void) {
    return (passgen_array_t) {
        .data = NULL,
        .len = 0,
        .bins = 0,
    };
}

void *passgen_array_push(passgen_array_t *array, size_t size, passgen_mem_t *mem) {
    // make sure we have a list of bins
    if(!array->bins) {
        array->data = passgen_malloc(mem, BINS_INITIAL * sizeof(void *));
        if(!array->data) {
            return NULL;
        } else {
            array->bins = BINS_INITIAL;
        }
    }

    // make sure the list of bins is large enough.
    if(array->len == (array->bins * ITEMS_PER_BIN(size))) {
        size_t new_bins = array->bins * BINS_MULTIPLIER;
        void **new_data = passgen_realloc(mem, array->data, new_bins * sizeof(void *));
        if(!new_data) {
            return NULL;
        } else {
            array->data = new_data;
            array->bins = new_bins;
        }
    }

    size_t bin = array->len / ITEMS_PER_BIN(size);
    size_t offset = array->len % ITEMS_PER_BIN(size);

    // make sure we have a bin allocated for this
    if(offset == 0) {
        array->data[bin] = passgen_malloc(mem, ITEMS_PER_BIN(size) * size);
        if(!array->data[bin]) {
            return NULL;
        }
    }

    array->len += 1;
    return (array->data[bin] + offset * size);
}

void *passgen_array_get(passgen_array_t *array, size_t size, size_t pos) {
    size_t bin = pos / ITEMS_PER_BIN(size);
    size_t offset = pos % ITEMS_PER_BIN(size);
    return (array->data[bin] + offset * size);
}

void passgen_array_free(passgen_array_t *array, size_t size, passgen_mem_t *mem) {
    size_t bins = (array->len + ITEMS_PER_BIN(size) - 1) / ITEMS_PER_BIN(size);

    assert(bins <= array->bins);

    for(size_t i = 0; i < bins; i++) {
        passgen_free(mem, array->data[i]);
    }

    if(bins) {
        passgen_free(mem, array->data);
    }
}
