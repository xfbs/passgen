#pragma once
#include "passgen/memory.h"

struct passgen_array {
    void **data;
    size_t len;
    size_t bins;
};

typedef struct passgen_array passgen_array_t;

passgen_array_t passgen_array_init(void);
void *passgen_array_push(passgen_array_t *array, size_t size, passgen_mem_t *mem);
void *passgen_array_get(passgen_array_t *array, size_t size, size_t pos);
void passgen_array_free(passgen_array_t *array, size_t size, passgen_mem_t *mem);
