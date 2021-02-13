#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <passgen/config.h>

struct passgen_context {
#ifdef PASSGEN_MEMORY
    void *(*malloc)(void *state, size_t size);
    void *(*calloc)(void *state, size_t count, size_t size);
    void *(*realloc)(void *state, void *ptr, size_t size);
    void (*free)(void *state, void *ptr);
    void *alloc_state;
#endif
};

typedef struct passgen_context passgen_context;

void passgen_context_init(passgen_context *ctx);

#ifdef PASSGEN_MEMORY
void passgen_context_set_malloc(passgen_context *ctx, void *(*malloc)(void *state, size_t size));
void passgen_context_set_calloc(passgen_context *ctx, void *(*calloc)(void *state, size_t count, size_t size));
void passgen_context_set_realloc(passgen_context *ctx, void *(*realloc)(void *state, void *ptr, size_t size));
void passgen_context_set_free(passgen_context *ctx, void (*free)(void *state, void *ptr));
void passgen_context_set_alloc_state(passgen_context *ctx, void *alloc_state);
#endif

#ifdef PASSGEN_MEMORY
void *passgen_malloc(const passgen_context *ctx, size_t size);
void *passgen_calloc(const passgen_context *ctx, size_t count, size_t size);
void *passgen_realloc(const passgen_context *ctx, void *ptr, size_t size);
void passgen_free(const passgen_context *ctx, void *ptr);
#else
#define passgen_malloc(ctx, size) malloc(size)
#define passgen_calloc(ctx, count, size) calloc(count, size)
#define passgen_realloc(ctx, ptr, size) realloc(ptr, size)
#define passgen_free(ctx, ptr) free(ptr)
#endif
