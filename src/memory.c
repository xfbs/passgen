#include "passgen/memory.h"

void *passgen_malloc(struct passgen_mem *mem, size_t size) {
    if(!mem) {
        return malloc(size);
    } else {
        return mem->malloc(mem->state, size);
    }
}

void passgen_free(struct passgen_mem *mem, void *ptr) {
    if(!mem) {
        free(ptr);
    } else {
        mem->free(mem->state, ptr);
    }
}

void *passgen_realloc(struct passgen_mem *mem, void *ptr, size_t size) {
    if(!mem) {
        return realloc(ptr, size);
    } else {
        return mem->realloc(mem->state, ptr, size);
    }
}

void *passgen_calloc(struct passgen_mem *mem, size_t size, size_t count) {
    if(!mem) {
        return calloc(size, count);
    } else {
        return mem->calloc(mem->state, size, count);
    }
}

static void *passgen_malloc_limited(void *state, size_t size) {
    struct passgen_mem_limits *limits = state;

    if(limits->allocs_limit && limits->allocs_limit == limits->allocs) {
        return NULL;
    }

    if(limits->size_limit && limits->size_limit < (limits->size + size)) {
        return NULL;
    }

    limits->allocs += 1;
    limits->size += size;

    return passgen_malloc(limits->mem, size);
}

static void *passgen_calloc_limited(void *state, size_t count, size_t size) {
    struct passgen_mem_limits *limits = state;

    if(limits->allocs_limit && limits->allocs_limit == limits->allocs) {
        return NULL;
    }

    if(limits->size_limit && limits->size_limit < (limits->size + count * size)) {
        return NULL;
    }

    limits->allocs += 1;
    limits->size += count * size;

    return passgen_calloc(limits->mem, count, size);
}

static void *passgen_realloc_limited(void *state, void *ptr, size_t size) {
    struct passgen_mem_limits *limits = state;

    if(limits->allocs_limit && limits->allocs_limit == limits->allocs) {
        return NULL;
    }

    if(limits->size_limit && limits->size_limit < (limits->size + size)) {
        return NULL;
    }

    limits->allocs += 1;
    limits->size += size;

    return passgen_realloc(limits->mem, ptr, size);
}

static void passgen_free_limited(void *state, void *ptr) {
    struct passgen_mem_limits *limits = state;

    passgen_free(limits->mem, ptr);
}

passgen_mem_t passgen_mem_limited(passgen_mem_limits_t *limits) {
    return (passgen_mem_t) {
        .malloc = passgen_malloc_limited,
        .realloc = passgen_realloc_limited,
        .calloc = passgen_calloc_limited,
        .free = passgen_free_limited,
        .state = limits,
    };
}

passgen_mem_limits_t passgen_mem_limits(passgen_mem_t *mem, size_t size, size_t allocs) {
    return (struct passgen_mem_limits) {
        .mem = mem,
        .size = 0,
        .size_limit = size,
        .allocs = 0,
        .allocs_limit = allocs,
    };
}
