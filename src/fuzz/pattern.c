/// @file pattern.c
///
/// Fuzzing harness for parsing a pattern and generating from it. The input is
/// the pattern.
#include "common.h"
#include <passgen/generate.h>
#include <passgen/passgen.h>
#include <passgen/env.h>
#include <stdio.h>
#include <stdlib.h>

static const char wordlist[] = "alpha\nbravo\ncharlie\ndelta\necho\nfoxtrot\n";

// What parsing returned, for the run as it is, and the one where an
// allocation fails.
static int parsed_runs[2];
static size_t run_index;

static void run(const uint8_t *data, size_t size) {
    // patterns are strings, so anything after a NUL byte is ignored.
    char *string = malloc(size + 1);
    if(!string) {
        return;
    }
    memcpy(string, data, size);
    string[size] = 0;

    passgen_pattern *pattern;
    passgen_error error;
    int parsed = passgen_parse(&pattern, &error, string);
    parsed_runs[run_index++] = parsed;

    if(parsed == 0) {
        // these are volatile as they are used after the longjmp().
        passgen_env *volatile env = NULL;
        passgen_random random;
        fuzz_random_context context;

        if(0 == setjmp(context.exhausted)) {
            fuzz_random_open(&random, &context);
            env = passgen_env_new(&random);

            // a wordlist named `w`, so that \w{w} and \m{w} do something.
            FILE *file = fmemopen((void *) wordlist, strlen(wordlist), "r");
            if(env && file) {
                // this closes the file, also if it fails.
                passgen_env_wordlist_add(env, "w", file, 2);
            } else if(file) {
                fclose(file);
            }

            if(env) {
                // with and without computing the entropy.
                double entropy;
                size_t budget = FUZZ_OUTPUT_BUDGET;
                passgen_generate(pattern, env, NULL, &entropy, &budget, fuzz_output);
                budget = FUZZ_OUTPUT_BUDGET;
                passgen_generate(pattern, env, NULL, NULL, &budget, fuzz_output);
            }
        }

        // the randomness source is on the stack, and not owned by the
        // environment.
        passgen_env_free(env);
    }

    passgen_pattern_free(pattern);
    free(string);
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if(fuzz_has_long_number(data, size)) {
        return 0;
    }

    run_index = 0;
    fuzz_run(run, data, size);

    // when an allocation fails, parsing either fails because of that, or
    // works out the same as it does otherwise (as when it was one which is
    // not needed for parsing).
    if(run_index == 2 && parsed_runs[1] != parsed_runs[0] &&
       parsed_runs[1] != PASSGEN_ERROR_OUT_OF_MEMORY) {
        abort();
    }

    return 0;
}
