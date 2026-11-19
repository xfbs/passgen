// generate.c
//
// Generates a password from a pattern, using all of what that takes: parsing
// the pattern, opening a source of randomness, setting up an environment
// with a wordlist, generating, and cleaning up, with the errors along the
// way.
//
//     generate [pattern [wordlist]]
//
// The wordlist is a file with one word per line, which the pattern can use
// as `\w{words}`. This is built and run as part of the tests.
#include <passgen/passgen.h>

#include <stdio.h>
#include <stdlib.h>

// Prints an error, and for errors in the pattern, marks where in the
// pattern it is.
static void print_error(const passgen_error *error, const char *pattern) {
    fprintf(stderr, "error: ");
    passgen_error_print(error, stderr);

    if(error->has_position) {
        // this assumes that every codepoint is one column wide. an error at
        // the end of the pattern (such as a set which is not closed) is
        // marked right after it.
        size_t start = error->start.codepoint;
        size_t end = error->end.codepoint > start ? error->end.codepoint
                                                  : start + 1;
        fprintf(stderr, "  %s\n  ", pattern);
        for(size_t i = 0; i < start; i++) {
            fputc(' ', stderr);
        }
        for(size_t i = start; i < end; i++) {
            fputc('^', stderr);
        }
        fputc('\n', stderr);
    }
}

int main(int argc, char *argv[]) {
    const char *string = argc > 1 ? argv[1] : "[a-z]{5}(-[a-z]{5}){3}";
    const char *wordlist = argc > 2 ? argv[2] : NULL;
    int ret = EXIT_FAILURE;

    // parsing fills in the error if the pattern is not valid. the error has
    // to be released once it was looked at.
    passgen_pattern *pattern = NULL;
    passgen_error error;
    if(passgen_parse(&pattern, &error, string) != PASSGEN_OK) {
        print_error(&error, string);
        passgen_error_free(&error);
        return EXIT_FAILURE;
    }

    // the source of randomness of the system, opened in place. it belongs to
    // whoever opened it, and is closed at the end.
    passgen_random random;
    if(!passgen_random_system_open(&random)) {
        fprintf(stderr, "error: cannot open the source of randomness\n");
        passgen_pattern_free(pattern);
        return EXIT_FAILURE;
    }

    // the environment has the source of randomness and the wordlists.
    passgen_env *env = passgen_env_new(&random);
    if(!env) {
        fprintf(stderr, "error: out of memory\n");
        goto close_random;
    }

    if(wordlist) {
        FILE *file = fopen(wordlist, "r");
        if(!file) {
            perror(wordlist);
            goto free_env;
        }

        // the environment takes over the file, and reads it when the pattern
        // is resolved. the 3 is the length of the Markov chain, for
        // \m{words}.
        if(passgen_env_wordlist_add(env, "words", file, 3) != PASSGEN_OK) {
            fprintf(stderr, "error: out of memory\n");
            goto free_env;
        }
    }

    // this reads the wordlists which the pattern uses, and fails if it uses
    // one which is not there. it is done once, however often the pattern is
    // generated afterwards.
    if(passgen_resolve(pattern, env, &error) != PASSGEN_OK) {
        print_error(&error, string);
        passgen_error_free(&error);
        goto free_env;
    }

    // what is generated is UTF-8. one byte is left for terminating it.
    uint8_t output[256];
    size_t length = 0;
    double entropy = 0;
    passgen_status status = passgen_generate_fill_utf8(
        pattern,
        env,
        &error,
        &entropy,
        output,
        sizeof(output) - 1,
        &length);
    if(status != PASSGEN_OK) {
        print_error(&error, string);
        passgen_error_free(&error);
        goto free_env;
    }
    output[length] = 0;

    printf("%s\n", (const char *) output);
    fprintf(stderr, "entropy: %.1f bits\n", entropy);
    ret = EXIT_SUCCESS;

free_env:
    passgen_env_free(env);
close_random:
    passgen_random_close(&random);
    passgen_pattern_free(pattern);
    return ret;
}
