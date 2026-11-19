/// @file wordlist.c
///
/// Fuzzing harness for wordlists and Markov chains. The first byte of the
/// input is the length of the Markov chain, the rest is the wordlist.
#include "common.h"
#include <passgen/env.h>
#include <passgen/generate.h>
#include <passgen/passgen.h>
#include <stdio.h>
#include <stdlib.h>

static void run(const uint8_t *data, size_t size) {
    // the length of the chain is kept small, as the memory needed grows with
    // it.
    size_t markov_length = 1 + data[0] % 4;
    data += 1;
    size -= 1;

    char string[32];
    snprintf(string, sizeof(string), "\\w{w}-\\m{w:%zu}{3}", markov_length);
    passgen_pattern *pattern;
    passgen_error error;
    int parsed = passgen_parse(&pattern, &error, string);
    if(parsed != 0) {
        // this only fails when an allocation does.
        if(parsed != PASSGEN_ERROR_OUT_OF_MEMORY) {
            abort();
        }
        return;
    }

    passgen_env *volatile env = NULL;
    passgen_random random;
    fuzz_random_context context;

    if(0 == setjmp(context.exhausted)) {
        fuzz_random_open(&random, &context);
        env = passgen_env_new(&random);

        FILE *file = fmemopen((void *) data, size, "r");
        if(env && file) {
            // this closes the file, also if it fails.
            passgen_env_wordlist_add(env, "w", file);
            passgen_resolve(pattern, env, NULL);

            double entropy;
            size_t budget = FUZZ_OUTPUT_BUDGET;
            passgen_generate(
                pattern,
                env,
                NULL,
                &entropy,
                &budget,
                fuzz_output);
        } else if(file) {
            fclose(file);
        }
    }

    // the randomness source is on the stack, and not owned by the
    // environment.
    passgen_env_free(env);

    passgen_pattern_free(pattern);
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if(size < 2) {
        return 0;
    }

    fuzz_run(run, data, size);

    return 0;
}
