/// @file wordlist.c
///
/// Fuzzing harness for wordlists and Markov chains. The first byte of the
/// input is the length of the Markov chain, the rest is the wordlist.
#include "common.h"
#include <passgen/generate.h>
#include <passgen/passgen.h>
#include <passgen/pattern/env.h>
#include <stdio.h>
#include <stdlib.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if(size < 2) {
        return 0;
    }

    // the length of the chain is kept small, as the memory needed grows with
    // it.
    size_t markov_length = 1 + data[0] % 4;
    data += 1;
    size -= 1;

    passgen_pattern pattern;
    passgen_error error;
    if(0 != passgen_parse(&pattern, &error, "\\w{w}-\\m{w}{3}")) {
        abort();
    }

    passgen_env *volatile env_ptr = NULL;
    passgen_random random;
    fuzz_random_context context;
    passgen_env env;

    if(0 == setjmp(context.exhausted)) {
        fuzz_random_open(&random, &context);
        passgen_env_init(&env, &random);
        env_ptr = &env;

        FILE *file = fmemopen((void *) data, size, "r");
        char *name = malloc(2);
        if(file && name) {
            name[0] = 'w';
            name[1] = 0;
            passgen_env_wordlist_add(&env, name, file, markov_length);

            double entropy;
            size_t budget = FUZZ_OUTPUT_BUDGET;
            passgen_generate(&pattern, &env, &entropy, &budget, fuzz_output);
        } else {
            if(file) fclose(file);
            free(name);
        }
    }

    if(env_ptr) {
        // the randomness source is on the stack.
        env_ptr->random = NULL;
        passgen_env_free(env_ptr);
    }

    passgen_pattern_free(&pattern);

    return 0;
}
