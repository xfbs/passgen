#pragma once
#include <stdlib.h>
#include <stdbool.h>

struct passgen_mem;

typedef void *passgen_malloc_fn(void *state, size_t size);
typedef void *passgen_calloc_fn(void *state, size_t count, size_t size);
typedef void *passgen_realloc_fn(void *state, void *ptr, size_t size);
typedef void passgen_free_fn(void *state, void *ptr);

struct passgen_mem {
    passgen_malloc_fn *malloc;
    passgen_calloc_fn *calloc;
    passgen_realloc_fn *realloc;
    passgen_free_fn *free;
    void *state;
};

struct passgen_mem_limits {
    struct passgen_mem *mem;
    size_t size;
    size_t size_limit;
    size_t allocs;
    size_t allocs_limit;
};

typedef struct passgen_mem passgen_mem_t;
typedef struct passgen_mem_limits passgen_mem_limits_t;

void *passgen_malloc(struct passgen_mem *mem, size_t size);
void *passgen_calloc(struct passgen_mem *mem, size_t count, size_t size);
void *passgen_realloc(struct passgen_mem *mem, void *ptr, size_t size);
void passgen_free(struct passgen_mem *mem, void *ptr);

passgen_mem_t passgen_mem_limited(passgen_mem_limits_t *limits);

passgen_mem_limits_t passgen_mem_limits(passgen_mem_t *mem, size_t size, size_t allocs);
