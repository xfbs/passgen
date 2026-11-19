/// @file common.h
///
/// Helpers shared by the fuzzing harnesses.
///
/// A pattern such as `(a{999}){999}` is short, but results in a lot of
/// output. To keep every run of a harness short, there is a budget for the
/// output, one for the randomness which may be consumed, and a lower step
/// limit than the default.
///
/// Every input is run twice, using an allocator which checks that everything
/// is released, with the size it was allocated with: once as it is, and once
/// with one of the allocations failing (see @ref fuzz_run).
#pragma once
#include <passgen/allocator.h>
#include <passgen/random.h>
#include <setjmp.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/// How many codepoints may be generated per run.
#define FUZZ_OUTPUT_BUDGET 4096

/// How many steps generating may take per run (see
/// passgen_env_step_limit_set()). This is what bounds patterns which take
/// long without generating anything, such as `(){999999999}`.
#define FUZZ_STEP_LIMIT 100000

/// How many bytes of randomness may be consumed per run.
#define FUZZ_RANDOM_BUDGET (256 * 1024)

/// Randomness source with a budget. Once it is used up, it jumps back to
/// where the budget was set, as a randomness source has no way to fail.
typedef struct {
    uint64_t state;
    size_t budget;
    jmp_buf exhausted;
} fuzz_random_context;

static inline size_t
fuzz_random_read(void *context_raw, void *dest, size_t size) {
    fuzz_random_context *context = context_raw;
    if(context->budget < size) {
        longjmp(context->exhausted, 1);
    }
    context->budget -= size;

    // xorshift, good enough to reach all kinds of choices
    uint8_t *output = dest;
    for(size_t i = 0; i < size; i++) {
        context->state ^= context->state << 13;
        context->state ^= context->state >> 7;
        context->state ^= context->state << 17;
        output[i] = (uint8_t) (context->state >> 32);
    }

    return size;
}

static inline void fuzz_random_close(void *context) {
    (void) context;
}

/// Sets up a randomness source using the context. The budget has to be set
/// (using setjmp() on `context->exhausted`) before this is called.
static inline void
fuzz_random_open(passgen_random *random, fuzz_random_context *context) {
    context->state = UINT64_C(0x9e3779b97f4a7c15);
    context->budget = FUZZ_RANDOM_BUDGET;
    random->context = context;
    random->read = fuzz_random_read;
    random->close = fuzz_random_close;
    random->stream = NULL;
    random->failed = false;
    passgen_random_reload(random);
}

/// Callback for passgen_generate() which stops when the output budget is used
/// up. The data is a pointer to a size_t with the remaining budget.
static inline int fuzz_output(void *data, uint32_t codepoint) {
    size_t *budget = data;
    (void) codepoint;
    if(*budget == 0) {
        return -1;
    }
    *budget -= 1;
    return 0;
}

/// Allocator which can be made to fail, and which checks that every
/// allocation is released with the size it was allocated with.
typedef struct {
    /// Number of allocations (including resizes) which were asked for.
    size_t requests;
    /// The first request which fails.
    size_t fail_at;
    /// How many requests fail, starting with that one.
    size_t fail_count;
    /// Number of allocations which have not been released.
    size_t live;
} fuzz_allocator;

// every allocation has a header which remembers its size, as large as the
// largest alignment, so that what follows it is aligned as well.
typedef union {
    size_t size;
    long double align_ld;
    long long align_ll;
    void *align_ptr;
} fuzz_alloc_header;

static bool fuzz_alloc_fails(fuzz_allocator *allocator) {
    size_t request = allocator->requests++;
    return request >= allocator->fail_at &&
           request - allocator->fail_at < allocator->fail_count;
}

static fuzz_alloc_header *fuzz_alloc_header_of(void *ptr, size_t size) {
    fuzz_alloc_header *block = (fuzz_alloc_header *) ptr - 1;
    if(block->size != size) {
        // released with the wrong size.
        abort();
    }
    return block;
}

static void *fuzz_alloc(void *state, size_t size) {
    fuzz_allocator *allocator = state;
    if(fuzz_alloc_fails(allocator)) {
        return NULL;
    }
    fuzz_alloc_header *block = malloc(sizeof(fuzz_alloc_header) + size);
    if(!block) {
        return NULL;
    }
    block->size = size;
    allocator->live += 1;
    return block + 1;
}

static void *
fuzz_alloc_resize(void *state, void *ptr, size_t old_size, size_t new_size) {
    fuzz_allocator *allocator = state;
    fuzz_alloc_header *block = fuzz_alloc_header_of(ptr, old_size);
    if(fuzz_alloc_fails(allocator)) {
        return NULL;
    }
    fuzz_alloc_header *moved =
        realloc(block, sizeof(fuzz_alloc_header) + new_size);
    if(!moved) {
        return NULL;
    }
    moved->size = new_size;
    return moved + 1;
}

static void fuzz_alloc_free(void *state, void *ptr, size_t size) {
    fuzz_allocator *allocator = state;
    fuzz_alloc_header *block = fuzz_alloc_header_of(ptr, size);
    allocator->live -= 1;
    free(block);
}

/// Runs the harness once with the allocator, and checks that everything was
/// released.
static inline void fuzz_run_once(
    void (*run)(const uint8_t *data, size_t size),
    const uint8_t *data,
    size_t size,
    fuzz_allocator *allocator) {
    passgen_allocator config = {
        .alloc = fuzz_alloc,
        .resize = fuzz_alloc_resize,
        .free = fuzz_alloc_free,
        .state = allocator,
    };
    passgen_allocator_set(&config);
    run(data, size);
    passgen_allocator_set(NULL);

    if(allocator->live != 0) {
        // something was not released.
        abort();
    }
}

/// Runs the harness on the input twice: once as it is, and once with one of
/// the allocations failing. Which one fails, and whether every one after it
/// fails as well (as when running out of memory) or not, depends on the
/// input, so that the same input always does the same.
static inline void fuzz_run(
    void (*run)(const uint8_t *data, size_t size),
    const uint8_t *data,
    size_t size) {
    fuzz_allocator allocator = {.fail_at = SIZE_MAX, .fail_count = SIZE_MAX};
    fuzz_run_once(run, data, size, &allocator);
    size_t requests = allocator.requests;
    if(requests == 0) {
        return;
    }

    // FNV-1a
    uint64_t hash = UINT64_C(0xcbf29ce484222325);
    for(size_t i = 0; i < size; i++) {
        hash = (hash ^ data[i]) * UINT64_C(0x100000001b3);
    }

    allocator = (fuzz_allocator){
        .fail_at = (size_t) (hash % requests),
        .fail_count = (hash >> 63) ? 1 : SIZE_MAX,
    };
    fuzz_run_once(run, data, size, &allocator);
}
