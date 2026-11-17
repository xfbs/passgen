/// @file regression.c
///
/// Regression tests for bugs that were found and fixed. Most of these are
/// only really meaningful when run with sanitizers enabled.
#include "tests.h"
#include <math.h>
#include <passgen/generate.h>
#include <passgen/passgen.h>
#include <passgen/pattern/env.h>
#include <passgen/random.h>
#include <stdlib.h>
#include <string.h>

/// Reads that straddle the end of the internal buffer must return the
/// remaining buffered bytes followed by fresh ones, rather than handing out
/// bytes from the start of the buffer a second time.
test_result test_regression_random_read_boundary(void) {
    passgen_random chunked, bytewise;
    assert(passgen_random_xorshift_open(&chunked, 1234));
    assert(passgen_random_xorshift_open(&bytewise, 1234));

    size_t offset = PASSGEN_RANDOM_BUFFER_LENGTH - 4;
    uint8_t expected[PASSGEN_RANDOM_BUFFER_LENGTH + 4];
    for(size_t i = 0; i < sizeof(expected); i++) {
        expected[i] = passgen_random_u8(&bytewise);
    }

    uint8_t skip[PASSGEN_RANDOM_BUFFER_LENGTH];
    uint8_t straddle[8];
    passgen_random_read(&chunked, skip, offset);
    passgen_random_read(&chunked, straddle, sizeof(straddle));

    assert(0 == memcmp(skip, expected, offset));
    assert(0 == memcmp(straddle, &expected[offset], sizeof(straddle)));

    // exactly emptying the buffer must work too.
    passgen_random_close(&chunked);
    assert(passgen_random_xorshift_open(&chunked, 1234));
    passgen_random_read(&chunked, skip, offset);
    passgen_random_read(&chunked, straddle, 4);
    assert(0 == memcmp(straddle, &expected[offset], 4));
    assert(passgen_random_u8(&chunked) == expected[offset + 4]);

    passgen_random_close(&chunked);
    passgen_random_close(&bytewise);

    return test_ok;
}

#ifdef PASSGEN_MONOCYPHER
/// Overlong keys and passphrases in randomness descriptions used to overflow
/// fixed-size stack buffers.
test_result test_regression_random_open_overlong(void) {
    char desc[1024];
    passgen_random random;

    // 32 bytes of key is fine, 33 is not.
    strcpy(desc, "chacha20:");
    memset(desc + strlen(desc), 'A', 32);
    desc[9 + 32] = 0;
    assert(passgen_random_open(&random, desc));
    passgen_random_close(&random);

    desc[9 + 32] = 'A';
    desc[9 + 33] = 0;
    assert(!passgen_random_open(&random, desc));

    memset(desc, 'A', sizeof(desc) - 1);
    desc[sizeof(desc) - 1] = 0;
    memcpy(desc, "chacha20:", 9);
    assert(!passgen_random_open(&random, desc));

    // same for the fields of chacha20-argon2.
    memset(desc, 'A', sizeof(desc) - 1);
    memcpy(desc, "chacha20-argon2:", 16);
    assert(!passgen_random_open(&random, desc));

    strcpy(desc, "chacha20-argon2:pass:");
    memset(desc + strlen(desc), 'B', 500);
    desc[21 + 500] = 0;
    assert(!passgen_random_open(&random, desc));

    assert(passgen_random_open(&random, "chacha20-argon2:pass:domain:token"));
    passgen_random_close(&random);

    return test_ok;
}
#endif

/// Repetitions where the maximum is smaller than the minimum, or that
/// overflow, must be rejected by the parser.
test_result test_regression_parse_repeat_range(void) {
    passgen_pattern pattern;
    passgen_error error;

    assert(0 != passgen_parse(&pattern, &error, "a{5,2}"));
    passgen_pattern_free(&pattern);

    assert(0 != passgen_parse(&pattern, &error, "a{1,0}"));
    passgen_pattern_free(&pattern);

    assert(
        0 != passgen_parse(&pattern, &error, "a{99999999999999999999999999}"));
    passgen_pattern_free(&pattern);

    assert(
        0 !=
        passgen_parse(&pattern, &error, "a{1,99999999999999999999999999}"));
    passgen_pattern_free(&pattern);

    assert(0 == passgen_parse(&pattern, &error, "a{2,5}"));
    passgen_pattern_free(&pattern);

    assert(0 == passgen_parse(&pattern, &error, "a{2,2}"));
    passgen_pattern_free(&pattern);

    return test_ok;
}

static int count_codepoints(void *data, uint32_t codepoint) {
    (void) codepoint;
    *((size_t *) data) += 1;
    return 0;
}

/// Optional items (`a?`) are a choice between two options, which needs to be
/// reflected in the entropy.
test_result test_regression_entropy_maybe(void) {
    passgen_pattern pattern;
    passgen_error error;
    passgen_env env;
    passgen_env_init(&env, passgen_random_open(NULL, "zero"));
    assert(env.random);

    double entropy = 0;
    size_t count = 0;

    assert(0 == passgen_parse(&pattern, &error, "a?"));
    assert(0 == passgen_generate(&pattern, &env, &entropy, &count, count_codepoints));
    assert(fabs(entropy - 2.0) < 0.0001);
    passgen_pattern_free(&pattern);

    assert(0 == passgen_parse(&pattern, &error, "a?[ab]?(c|d)?"));
    assert(0 == passgen_generate(&pattern, &env, &entropy, &count, count_codepoints));
    // with the zero randomness source, every optional item is emitted, so
    // this is 2 * (2 * 2) * (2 * 2).
    assert(fabs(entropy - 32.0) < 0.0001);
    passgen_pattern_free(&pattern);

    passgen_env_free(&env);

    return test_ok;
}

/// The JSON output must escape all control characters.
test_result test_regression_json_control_chars(void) {
    passgen_pattern pattern;
    passgen_error error;
    passgen_env env;
    passgen_env_init(&env, passgen_random_open(NULL, "zero"));
    assert(env.random);

    // literal control character U+0001 and a tab, a quote and a backslash
    assert(0 == passgen_parse(&pattern, &error, "\x01\t\"[\\\\]"));

    uint8_t buffer[64] = {0};
    size_t len = passgen_generate_fill_json_utf8(
        &pattern,
        &env,
        NULL,
        buffer,
        sizeof(buffer) - 1);
    buffer[len] = 0;
    assert_streq((const char *) buffer, "\\u0001\\t\\\"\\\\");

    passgen_pattern_free(&pattern);
    passgen_env_free(&env);

    return test_ok;
}

#ifndef _WIN32 // fmemopen does not exist on windows

static char *copy_string(const char *str) {
    char *copy = malloc(strlen(str) + 1);
    strcpy(copy, str);
    return copy;
}

/// Sets up an environment with a single wordlist named `list`.
static bool wordlist_env(
    passgen_env *env,
    const char *random,
    char *data,
    size_t len,
    size_t level) {
    // fmemopen() with an empty buffer is not portable, so use an empty file
    // in that case.
    FILE *file = len ? fmemopen(data, len, "r") : tmpfile();
    if(!file) {
        return false;
    }

    passgen_env_init(env, passgen_random_open(NULL, random));
    if(!env->random) {
        return false;
    }

    return 0 == passgen_env_wordlist_add(env, copy_string("list"), file, level);
}

/// Picking from an empty wordlist used to hang (release builds) or trip an
/// assertion (debug builds). It should just be an error.
test_result test_regression_wordlist_empty(void) {
    passgen_pattern pattern;
    passgen_error error;
    passgen_env env;
    size_t count = 0;

    assert(wordlist_env(&env, "zero", NULL, 0, 3));
    assert(0 == passgen_parse(&pattern, &error, "\\w{list}"));
    assert(0 != passgen_generate(&pattern, &env, NULL, &count, count_codepoints));
    passgen_pattern_free(&pattern);

    assert(0 == passgen_parse(&pattern, &error, "\\m{list}"));
    assert(0 != passgen_generate(&pattern, &env, NULL, &count, count_codepoints));
    passgen_pattern_free(&pattern);
    passgen_env_free(&env);

    // wordlist that does not exist at all
    passgen_env_init(&env, passgen_random_open(NULL, "zero"));
    assert(0 == passgen_parse(&pattern, &error, "\\w{nope}\\m{nope}"));
    assert(0 != passgen_generate(&pattern, &env, NULL, &count, count_codepoints));
    passgen_pattern_free(&pattern);
    passgen_env_free(&env);

    return test_ok;
}

/// Words longer than 128 codepoints used to fail (wordlist) or overflow a
/// stack buffer (markov chain).
test_result test_regression_wordlist_long_words(void) {
    passgen_pattern pattern;
    passgen_error error;
    passgen_env env;

    // a single word consisting of 600 times the letter a.
    size_t word_len = 600;
    char *data = malloc(word_len + 1);
    memset(data, 'a', word_len);
    data[word_len] = '\n';

    assert(wordlist_env(&env, "xorshift:1234", data, word_len + 1, 1));

    size_t count = 0;
    assert(0 == passgen_parse(&pattern, &error, "\\w{list}"));
    assert(0 == passgen_generate(&pattern, &env, NULL, &count, count_codepoints));
    assert_eq(count, word_len);
    passgen_pattern_free(&pattern);

    // at every step, the markov chain has a 599/600 chance of generating
    // another a, so these words get long: on average 600 codepoints.
    count = 0;
    double entropy = 0;
    assert(0 == passgen_parse(&pattern, &error, "\\m{list}{100}"));
    assert(
        0 ==
        passgen_generate(&pattern, &env, &entropy, &count, count_codepoints));
    assert(count > 128 * 100);
    assert(entropy > 1.0);
    passgen_pattern_free(&pattern);

    passgen_env_free(&env);
    free(data);

    return test_ok;
}

/// Defining a wordlist twice replaces the first one (and must not leak it).
test_result test_regression_wordlist_redefine(void) {
    passgen_pattern pattern;
    passgen_error error;
    passgen_env env;
    char first[] = "first\n";
    char second[] = "second\n";

    assert(wordlist_env(&env, "zero", first, strlen(first), 3));

    FILE *file = fmemopen(second, strlen(second), "r");
    assert(file);
    assert(0 == passgen_env_wordlist_add(&env, copy_string("list"), file, 3));

    uint8_t buffer[32] = {0};
    assert(0 == passgen_parse(&pattern, &error, "\\w{list}"));
    size_t len = passgen_generate_fill_utf8(
        &pattern,
        &env,
        NULL,
        buffer,
        sizeof(buffer) - 1);
    buffer[len] = 0;
    assert_streq((const char *) buffer, "second");

    passgen_pattern_free(&pattern);
    passgen_env_free(&env);

    return test_ok;
}

#endif
