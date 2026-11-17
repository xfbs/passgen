/// @file common.h
///
/// Helpers shared by the fuzzing harnesses.
///
/// Generating from a pattern is not bounded: a pattern such as `(a{999}){999}`
/// is short, but results in a lot of output. To keep every run of a harness
/// short, there is a budget for the output, and one for the randomness which
/// may be consumed.
#pragma once
#include <passgen/random.h>
#include <setjmp.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/// How many codepoints may be generated per run.
#define FUZZ_OUTPUT_BUDGET 4096

/// How many bytes of randomness may be consumed per run.
#define FUZZ_RANDOM_BUDGET (256 * 1024)

/// Randomness source with a budget. Once it is used up, it jumps back to
/// where the budget was set, as a randomness source has no way to fail.
typedef struct {
    uint64_t state;
    size_t budget;
    jmp_buf exhausted;
} fuzz_random_context;

static size_t fuzz_random_read(void *context_raw, void *dest, size_t size) {
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

static void fuzz_random_close(void *context) {
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

/// Repetitions are parsed as numbers, and a pattern such as `[]{999999999}`
/// loops that many times without generating output or consuming randomness.
/// So inputs with long numbers in them are skipped.
static inline int fuzz_has_long_number(const uint8_t *data, size_t size) {
    size_t digits = 0;
    for(size_t i = 0; i < size; i++) {
        digits = (data[i] >= '0' && data[i] <= '9') ? digits + 1 : 0;
        if(digits > 4) {
            return 1;
        }
    }
    return 0;
}
