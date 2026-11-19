/// @file public.c
///
/// Uses the library the way that something outside of this repository would:
/// with only the public headers, and linked against the shared library, which
/// only exports the public interface. If this does not build, something that
/// is needed is not public. Unlike the other tests, this does not use the
/// testing framework, as that is built with the internal headers.
#include <passgen/env.h>
#include <passgen/error.h>
#include <passgen/generate.h>
#include <passgen/passgen.h>
#include <passgen/random.h>
#include <passgen/version.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define check(condition)                                              \
    do {                                                              \
        if(!(condition)) {                                            \
            fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition); \
            exit(EXIT_FAILURE);                                       \
        }                                                             \
    } while(0)

static int count(void *data, uint32_t codepoint) {
    (void) codepoint;
    *((size_t *) data) += 1;
    return 0;
}

int main(void) {
    // the headers and the library are of the same version.
    passgen_version version = passgen_version_get();
    check(version.major == PASSGEN_VERSION_MAJOR);
    check(version.minor == PASSGEN_VERSION_MINOR);
    check(version.patch == PASSGEN_VERSION_PATCH);
    check(passgen_version_compatible(version));

    // a pattern which is not valid results in an error, and no pattern.
    passgen_pattern *pattern;
    passgen_error error;
    // the default allocator can be put back, which it already is.
    passgen_allocator_set(NULL);

    check(PASSGEN_ERROR_SET_UNCLOSED == passgen_parse(&pattern, &error, "[a-z"));
    check(pattern == NULL);
    check(error.kind == PASSGEN_ERROR_SET_UNCLOSED);
    check(error.has_position);
    check(error.start.codepoint == 4);
    check(0 == strcmp(passgen_status_string(error.kind), "set is not closed"));
    passgen_error_free(&error);
    passgen_pattern_free(pattern);

    // the error is not needed to find out whether it worked.
    check(PASSGEN_OK != passgen_parse(&pattern, NULL, "(a"));

    check(0 == passgen_parse(&pattern, &error, "\\w{words}-[a-z]{4}"));
    check(pattern != NULL);

    // the source of randomness is owned by whoever opens it.
    passgen_random random;
    check(passgen_random_open(&random, "xorshift:1234"));

    passgen_env *env = passgen_env_new(&random);
    check(env != NULL);

    // without the wordlist, this cannot be resolved, and so not generated.
    uint8_t output[64];
    size_t length = 0;
    check(
        PASSGEN_ERROR_WORDLIST_NOT_FOUND ==
        passgen_resolve(pattern, env, &error));
    check(error.kind == PASSGEN_ERROR_WORDLIST_NOT_FOUND);
    check(0 == strcmp(error.detail, "words"));
    passgen_error_free(&error);

    FILE *file = tmpfile();
    check(file != NULL);
    fputs("alpha\nbravo\ncharlie\n", file);
    check(0 == passgen_env_wordlist_add(env, "words", file, 3));

    double entropy = 0;
    check(PASSGEN_OK == passgen_resolve(pattern, env, NULL));
    passgen_generate_fill_utf8(
        pattern,
        env,
        NULL,
        &entropy,
        output,
        63,
        &length);
    output[length] = 0;
    check(length >= strlen("alpha-abcd"));
    check(strchr((const char *) output, '-') != NULL);
    // in bits: one of three words, and four letters. that is 20.39 bits.
    check(entropy > 20.38);
    check(entropy < 20.40);

    uint32_t codepoints[64];
    check(
        PASSGEN_OK ==
        passgen_generate_fill_unicode(pattern, env, NULL, NULL, codepoints, 64, &length));
    check(length >= strlen("alpha-abcd"));
    check(
        PASSGEN_OK ==
        passgen_generate_fill_json_utf8(pattern, env, NULL, NULL, output, 63, &length));
    check(length >= strlen("alpha-abcd"));

    // a buffer which is too small is an error, and not the same as nothing
    // being generated.
    check(
        PASSGEN_ERROR_BUFFER_TOO_SMALL ==
        passgen_generate_fill_utf8(pattern, env, NULL, NULL, output, 3, &length));
    check(length == 0);

    size_t codepoint_count = 0;
    check(0 == passgen_generate(pattern, env, NULL, NULL, &codepoint_count, count));
    check(codepoint_count >= strlen("alpha-abcd"));

    // with a step limit of one, this cannot be generated.
    passgen_env_step_limit_set(env, 1);
    check(
        PASSGEN_ERROR_STEP_LIMIT ==
        passgen_generate(pattern, env, NULL, NULL, &codepoint_count, count));
    passgen_env_step_limit_set(env, 1000);

    // which descriptions of sources of randomness may hold a secret, which
    // the command-line tool uses to not print them.
    check(passgen_random_desc_secret("chacha20-argon2:master"));
    check(passgen_random_desc_secret("chacha:mistyped"));
    check(!passgen_random_desc_secret("xorshift:1234"));

    // another source of randomness can be set later.
    passgen_random *zero = passgen_random_open(NULL, "zero");
    check(zero != NULL);
    passgen_env_random_set(env, zero);
    passgen_generate_fill_utf8(
        pattern,
        env,
        NULL,
        NULL,
        output,
        63,
        &length);
    output[length] = 0;
    check(0 == strcmp((const char *) output, "alpha-aaaa"));

    passgen_env_free(env);
    passgen_env_free(NULL);
    passgen_pattern_free(pattern);

    // the environment did not close these.
    (void) passgen_random_u64(&random);
    check(passgen_random_u8_max(zero, 10) == 0);
    passgen_random_close(&random);
    passgen_random_free(zero);

#ifdef PASSGEN_MONOCYPHER
    // master passphrase mode, with the default parameters and explicit ones.
    passgen_argon2_params params = PASSGEN_ARGON2_PARAMS_V1;
    check(params.memory == PASSGEN_ARGON2_PARAMS_DEFAULT.memory);
    check(passgen_random_chacha20_argon2_open(
        &random,
        "mypassword",
        "google.com",
        "",
        &params));
    check(passgen_random_u8(&random) == 42);
    passgen_random_close(&random);
#endif

    printf("public interface works\n");

    return EXIT_SUCCESS;
}
