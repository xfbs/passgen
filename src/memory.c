#include "passgen/memory.h"

void *passgen_malloc(struct passgen_mem *mem, size_t size) {
    if(!mem->malloc) {
        return malloc(size);
    } else {
        return mem->malloc(mem->state, size);
    }
}

void passgen_free(struct passgen_mem *mem, void *ptr) {
    if(!mem->free) {
        free(ptr);
    } else {
        mem->free(mem->state, ptr);
    }
}

void *passgen_realloc(struct passgen_mem *mem, void *ptr, size_t size) {
    if(!mem->malloc) {
        return realloc(ptr, size);
    } else {
        return mem->realloc(mem->state, ptr, size);
    }
}

void *passgen_calloc(struct passgen_mem *mem, size_t size, size_t count) {
    if(!mem->malloc) {
        return calloc(size, count);
    } else {
        return mem->calloc(mem->state, size, count);
    }
}

passgen_mem_t passgen_mem_stdlib(void) {
    return (passgen_mem_t) {
        .malloc = NULL,
        .realloc = NULL,
        .calloc = NULL,
        .free = NULL,
        .state = NULL,
    };
}
