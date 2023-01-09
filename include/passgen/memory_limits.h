#pragma once
#include <stddef.h>

struct passgen_mem_limits {
  struct passgen_mem *mem;
  size_t size;
  size_t size_limit;
  size_t allocs;
  size_t allocs_limit;
};

typedef struct passgen_mem_limits passgen_mem_limits_t;
