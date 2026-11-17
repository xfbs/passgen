#include "passgen/util/alloc.h"
#include <stdlib.h>
#include <string.h>

static void *system_alloc(void *state, size_t size) {
    (void) state;
    return malloc(size);
}

static void *
system_resize(void *state, void *ptr, size_t old_size, size_t new_size) {
    (void) state;
    (void) old_size;
    return realloc(ptr, new_size);
}

static void system_free(void *state, void *ptr, size_t size) {
    (void) state;
    (void) size;
    free(ptr);
}

static const passgen_allocator allocator_system = {
    .alloc = system_alloc,
    .resize = system_resize,
    .free = system_free,
    .state = NULL,
};

// the allocator which is in use. this is a copy, so that it does not matter
// what happens to the one which was passed in.
static passgen_allocator allocator_current = {
    .alloc = system_alloc,
    .resize = system_resize,
    .free = system_free,
    .state = NULL,
};

void passgen_allocator_set(const passgen_allocator *allocator) {
    allocator_current = allocator ? *allocator : allocator_system;
}

// allocators are never asked for nothing, as what they would return for that
// (NULL or not) is not something to rely on.
static inline size_t nonzero(size_t size) {
    return size ? size : 1;
}

void *passgen_alloc(size_t size) {
    return allocator_current.alloc(allocator_current.state, nonzero(size));
}

void *passgen_alloc_zeroed(size_t size) {
    void *ptr = passgen_alloc(size);
    if(ptr) {
        memset(ptr, 0, size);
    }
    return ptr;
}

void *passgen_resize(void *ptr, size_t old_size, size_t new_size) {
    if(!ptr) {
        return passgen_alloc(new_size);
    }

    return allocator_current.resize(
        allocator_current.state,
        ptr,
        nonzero(old_size),
        nonzero(new_size));
}

void passgen_free(void *ptr, size_t size) {
    if(ptr) {
        allocator_current.free(allocator_current.state, ptr, nonzero(size));
    }
}

char *passgen_alloc_string(const char *string) {
    size_t size = strlen(string) + 1;
    char *copy = passgen_alloc(size);
    if(copy) {
        memcpy(copy, string, size);
    }
    return copy;
}

void passgen_free_string(char *string) {
    if(string) {
        passgen_free(string, strlen(string) + 1);
    }
}
