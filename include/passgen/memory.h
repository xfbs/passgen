#pragma once
#include <passgen/data/memory.h>
#include <passgen/data/memory_accounting.h>
#include <passgen/data/memory_limits.h>
#include <stdbool.h>
#include <stddef.h>

void *passgen_malloc(struct passgen_mem *mem, size_t size);
void *passgen_calloc(struct passgen_mem *mem, size_t count, size_t size);
void *passgen_realloc(struct passgen_mem *mem, void *ptr, size_t size);
void passgen_free(struct passgen_mem *mem, void *ptr);

passgen_mem_t passgen_mem_limited(passgen_mem_limits_t *limits);

passgen_mem_limits_t
passgen_mem_limits(passgen_mem_t *mem, size_t size, size_t allocs);

passgen_mem_accounting_t passgen_mem_accounting_new(passgen_mem_t *mem);

passgen_mem_t passgen_mem_accounting(passgen_mem_accounting_t *acc);

void passgen_mem_accounting_cleanup(passgen_mem_accounting_t *acc);

bool passgen_mem_accounting_check(passgen_mem_accounting_t *acc);
