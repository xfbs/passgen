#include "passgen/context.h"
#include <string.h>

void passgen_context_init(passgen_context *ctx) {
    memset(ctx, 0, sizeof(passgen_context));
}

void passgen_context_set_malloc(
    passgen_context *ctx,
    void *(*malloc)(void *state, size_t size)) {
    ctx->malloc = malloc;
}

void passgen_context_set_calloc(
    passgen_context *ctx,
    void *(*calloc)(void *state, size_t count, size_t size)) {
    ctx->calloc = calloc;
}

void passgen_context_set_realloc(
    passgen_context *ctx,
    void *(*realloc)(void *state, void *ptr, size_t size)) {
    ctx->realloc = realloc;
}

void passgen_context_set_free(
    passgen_context *ctx,
    void (*free)(void *state, void *ptr)) {
    ctx->free = free;
}

void passgen_context_set_alloc_state(passgen_context *ctx, void *alloc_state) {
    ctx->alloc_state = alloc_state;
}

void *passgen_malloc(const passgen_context *ctx, size_t size);
void *passgen_calloc(const passgen_context *ctx, size_t count, size_t size);
void *passgen_realloc(const passgen_context *ctx, void *ptr, size_t size);
void passgen_free(const passgen_context *ctx, void *ptr);
