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
#include <passgen/util/utf8.h>
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

/// When the maximum is a power of two, no value ever has to be rejected, so
/// exactly one value is read from the randomness source per call. The mask
/// used to be one bit too wide in this case, which rejected half of them.
test_result test_regression_random_max_power_of_two(void) {
    passgen_random limited, plain;
    assert(passgen_random_xorshift_open(&limited, 1234));
    assert(passgen_random_xorshift_open(&plain, 1234));

    // if a value was rejected, the two sources would no longer be in sync.
    for(size_t i = 0; i < 1000; i++) {
        assert(passgen_random_u8_max(&limited, 1) == 0);
        passgen_random_u8(&plain);

        assert(
            passgen_random_u8_max(&limited, 16) ==
            (passgen_random_u8(&plain) & 15));
        assert(
            passgen_random_u16_max(&limited, 1024) ==
            (passgen_random_u16(&plain) & 1023));
        assert(
            passgen_random_u32_max(&limited, UINT32_C(1) << 20) ==
            (passgen_random_u32(&plain) & ((UINT32_C(1) << 20) - 1)));
        assert(
            passgen_random_u64_max(&limited, UINT64_C(1) << 40) ==
            (passgen_random_u64(&plain) & ((UINT64_C(1) << 40) - 1)));
    }

    // other maximums still only return values below the maximum.
    for(size_t i = 0; i < 1000; i++) {
        assert(passgen_random_u8_max(&limited, 3) < 3);
        assert(passgen_random_u8_max(&limited, 255) < 255);
        assert(passgen_random_u64_max(&limited, 62) < 62);
    }

    passgen_random_close(&limited);
    passgen_random_close(&plain);

    return test_ok;
}

/// The seed of the xorshift source is an unsigned 64 bit number, but it was
/// parsed as a signed one, so the upper half of the range did not work. It
/// also accepted anything that started with a number.
test_result test_regression_random_xorshift_seed(void) {
    passgen_random parsed, direct;

    assert(passgen_random_open(&parsed, "xorshift:18446744073709551615"));
    assert(passgen_random_xorshift_open(&direct, UINT64_MAX));
    assert(passgen_random_u64(&parsed) == passgen_random_u64(&direct));
    passgen_random_close(&parsed);
    passgen_random_close(&direct);

    assert(passgen_random_open(&parsed, "xorshift:9223372036854775808"));
    assert(passgen_random_xorshift_open(&direct, UINT64_C(9223372036854775808)));
    assert(passgen_random_u64(&parsed) == passgen_random_u64(&direct));
    passgen_random_close(&parsed);
    passgen_random_close(&direct);

    assert(!passgen_random_open(&parsed, "xorshift:"));
    assert(!passgen_random_open(&parsed, "xorshift:0"));
    assert(!passgen_random_open(&parsed, "xorshift:-1"));
    assert(!passgen_random_open(&parsed, "xorshift: 1"));
    assert(!passgen_random_open(&parsed, "xorshift:abc"));
    assert(!passgen_random_open(&parsed, "xorshift:12abc"));
    assert(!passgen_random_open(&parsed, "xorshift:18446744073709551616"));

    return test_ok;
}

/// Input which ends in the middle of a UTF-8 sequence used to be accepted,
/// with the incomplete sequence silently left out.
test_result test_regression_utf8_truncated(void) {
    // "ab", followed by the first two bytes of a three byte sequence
    const uint8_t truncated[] = {'a', 'b', 0xe6, 0x97};
    uint32_t codepoints[8];

    uint32_t *output = &codepoints[0];
    const uint8_t *input = &truncated[0];
    int ret = passgen_utf8_decode(&output, 8, NULL, &input, sizeof(truncated));
    assert(ret == PASSGEN_UTF8_INVALID_CHAR);

    // what came before the incomplete sequence is still decoded.
    assert(output == &codepoints[2]);
    assert(input == &truncated[2]);
    assert(codepoints[0] == 'a');
    assert(codepoints[1] == 'b');

    // the complete sequence is fine.
    const uint8_t complete[] = {'a', 'b', 0xe6, 0x97, 0xa5};
    output = &codepoints[0];
    input = &complete[0];
    ret = passgen_utf8_decode(&output, 8, NULL, &input, sizeof(complete));
    assert(ret == PASSGEN_UTF8_SUCCESS);
    assert(output == &codepoints[3]);
    assert(codepoints[2] == 0x65e5);

    // a pattern which ends this way is not valid.
    passgen_pattern pattern;
    passgen_error error;
    assert(0 != passgen_parse(&pattern, &error, "ab\xe6\x97"));
    passgen_pattern_free(&pattern);

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
