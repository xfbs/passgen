#pragma once
#include <stddef.h>

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

typedef struct passgen_mem passgen_mem_t;
