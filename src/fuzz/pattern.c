/// @file pattern.c
///
/// Fuzzing harness for parsing a pattern and generating from it. The input is
/// the pattern.
#include "common.h"
#include <passgen/generate.h>
#include <passgen/passgen.h>
#include <passgen/pattern/env.h>
#include <stdio.h>
#include <stdlib.h>

static const char wordlist[] = "alpha\nbravo\ncharlie\ndelta\necho\nfoxtrot\n";

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if(fuzz_has_long_number(data, size)) {
        return 0;
    }

    // patterns are strings, so anything after a NUL byte is ignored.
    char *string = malloc(size + 1);
    if(!string) {
        return 0;
    }
    memcpy(string, data, size);
    string[size] = 0;

    passgen_pattern pattern;
    passgen_error error;
    int parsed = passgen_parse(&pattern, &error, string);

    if(parsed == 0) {
        // these are volatile as they are used after the longjmp().
        passgen_env *volatile env_ptr = NULL;
        passgen_random random;
        fuzz_random_context context;
        passgen_env env;

        if(0 == setjmp(context.exhausted)) {
            fuzz_random_open(&random, &context);
            passgen_env_init(&env, &random);
            env_ptr = &env;

            // a wordlist named `w`, so that \w{w} and \m{w} do something.
            FILE *file = fmemopen((void *) wordlist, strlen(wordlist), "r");
            char *name = malloc(2);
            if(file && name) {
                name[0] = 'w';
                name[1] = 0;
                passgen_env_wordlist_add(&env, name, file, 2);
            } else {
                if(file) fclose(file);
                free(name);
            }

            // with and without computing the entropy.
            double entropy;
            size_t budget = FUZZ_OUTPUT_BUDGET;
            passgen_generate(&pattern, &env, &entropy, &budget, fuzz_output);
            budget = FUZZ_OUTPUT_BUDGET;
            passgen_generate(&pattern, &env, NULL, &budget, fuzz_output);
        }

        if(env_ptr) {
            // the randomness source is on the stack.
            env_ptr->random = NULL;
            passgen_env_free(env_ptr);
        }
    }

    passgen_pattern_free(&pattern);
    free(string);

    return 0;
}
