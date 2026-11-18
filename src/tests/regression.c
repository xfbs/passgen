/// @file regression.c
///
/// Regression tests for bugs that were found and fixed. Most of these are
/// only really meaningful when run with sanitizers enabled.
#include "tests.h"
#include <math.h>
#include <passgen/generate.h>
#include <passgen/passgen.h>
#include <passgen/pattern/pattern.h>
#include <passgen/pattern/segment.h>
#include <passgen/pattern/range.h>
#include <passgen/pattern/segment_item.h>
#include <passgen/pattern/set.h>
#include <passgen/env.h>
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
    passgen_pattern *pattern;
    passgen_error error;
    assert(0 != passgen_parse(&pattern, &error, "ab\xe6\x97"));
    passgen_pattern_free(pattern);

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
    passgen_pattern *pattern;
    passgen_error error;

    assert(0 != passgen_parse(&pattern, &error, "a{5,2}"));
    passgen_pattern_free(pattern);

    assert(0 != passgen_parse(&pattern, &error, "a{1,0}"));
    passgen_pattern_free(pattern);

    assert(
        0 != passgen_parse(&pattern, &error, "a{99999999999999999999999999}"));
    passgen_pattern_free(pattern);

    assert(
        0 !=
        passgen_parse(&pattern, &error, "a{1,99999999999999999999999999}"));
    passgen_pattern_free(pattern);

    assert(0 == passgen_parse(&pattern, &error, "a{2,5}"));
    passgen_pattern_free(pattern);

    assert(0 == passgen_parse(&pattern, &error, "a{2,2}"));
    passgen_pattern_free(pattern);

    return test_ok;
}

/// Giving a segment more than one multiplier (as in `{4}{1}a`) used to leave
/// the group with a sum of multipliers which did not match the segments, so
/// that choosing a segment could run past the last one.
test_result test_regression_parse_multiplier_twice(void) {
    passgen_pattern *pattern;
    passgen_error error;

    assert(0 == passgen_parse(&pattern, &error, "{4}{1}a|b"));
    assert(pattern->group.multiplier_sum == 2);
    passgen_pattern_free(pattern);

    assert(0 == passgen_parse(&pattern, &error, "{1}{4}a|{2}b"));
    assert(pattern->group.multiplier_sum == 6);
    passgen_pattern_free(pattern);

    // the sum must not overflow either.
    assert(
        0 != passgen_parse(
                 &pattern,
                 &error,
                 "{18446744073709551615}a|{18446744073709551615}b"));
    passgen_pattern_free(pattern);

    return test_ok;
}

/// A segment with a zero multiplier is recycled at the next `|`, which sets
/// its multiplier back to one. The sum of the group used to not count it
/// again, so that `({0}a|b)` generated nothing and `({0}a|b|c)` never chose
/// the last segment.
test_result test_regression_parse_multiplier_zero_recycled(void) {
    passgen_pattern *pattern;
    passgen_error error;

    assert(0 == passgen_parse(&pattern, &error, "{0}a|b"));
    assert(pattern->group.segments.len == 1);
    assert(pattern->group.multiplier_sum == 1);
    passgen_pattern_free(pattern);

    assert(0 == passgen_parse(&pattern, &error, "{0}a|b|c"));
    assert(pattern->group.segments.len == 2);
    assert(pattern->group.multiplier_sum == 2);
    passgen_pattern_free(pattern);

    assert(0 == passgen_parse(&pattern, &error, "{0}a|{3}b|{0}c|d"));
    assert(pattern->group.segments.len == 2);
    assert(pattern->group.multiplier_sum == 4);
    passgen_pattern_free(pattern);

    // the same holds within a nested group.
    assert(0 == passgen_parse(&pattern, &error, "({0}a|b|c)"));
    passgen_pattern_segment *segment =
        passgen_pattern_group_segment_get(&pattern->group, 0);
    passgen_pattern_item *item = passgen_stack_get(&segment->items, 0);
    assert(item->kind == PASSGEN_PATTERN_GROUP);
    assert(item->data.group.segments.len == 2);
    assert(item->data.group.multiplier_sum == 2);
    passgen_pattern_free(pattern);

    return test_ok;
}

/// Parses a pattern which consists of a single set, and checks that it has
/// exactly the given ranges, as triples of start, end and multiplier.
static bool set_ranges_are(const char *input, const uint32_t *ranges, size_t len) {
    passgen_pattern *pattern;
    passgen_error error;
    if(0 != passgen_parse(&pattern, &error, input)) {
        return false;
    }
    passgen_pattern_segment *segment =
        passgen_pattern_group_segment_get(&pattern->group, 0);
    passgen_pattern_item *item = passgen_stack_get(&segment->items, 0);
    bool same = segment->items.len == 1 && item->kind == PASSGEN_PATTERN_SET &&
                item->data.set.items.len == len;
    for(size_t i = 0; same && i < len; i++) {
        passgen_pattern_range *range = passgen_stack_get(&item->data.set.items, i);
        same = range->start == ranges[3 * i] && range->end == ranges[3 * i + 1] &&
               range->multiplier == ranges[3 * i + 2];
    }
    passgen_pattern_free(pattern);
    return same;
}

/// A dash after a range (as in `[a-c-e]`) used to change the end of that
/// range, so that `[a-zA-Z0-9-_]` silently contained `0` to `_`, and `[a-c-b]`
/// shrank to `[a-b]`. A dash is now only ever part of a range, and has to be
/// escaped to be the character itself.
test_result test_regression_parse_set_dash(void) {
    passgen_pattern *pattern;
    passgen_error error;

    const char *broken[] = {
        "[-]", "[-a]", "[a-]", "[a-c-]", "[a-c-e]", "[a-c-b]", "[a-zA-Z0-9-_]",
        NULL};
    for(size_t i = 0; broken[i]; i++) {
        assert(
            PASSGEN_ERROR_SET_RANGE ==
            passgen_parse(&pattern, &error, broken[i]));
    }

    // (the ranges of a set are sorted by codepoint.)
    const uint32_t dash[] = {'-', '-', 1};
    assert(set_ranges_are("[\\-]", dash, 1));

    const uint32_t range_dash_e[] = {'-', '-', 1, 'a', 'c', 1, 'e', 'e', 1};
    assert(set_ranges_are("[a-c\\-e]", range_dash_e, 3));

    const uint32_t url_safe[] = {
        '-', '-', 1, '0', '9', 1, 'A', 'Z', 1, '_', '_', 1, 'a', 'z', 1};
    assert(set_ranges_are("[a-zA-Z0-9\\-_]", url_safe, 5));

    // an escaped dash can start or end a range. here, both contain it.
    const uint32_t dash_range[] = {'+', ',', 1, '-', '-', 2, '.', '/', 1};
    assert(set_ranges_are("[\\--/+-\\-]", dash_range, 3));

    return test_ok;
}

/// The ranges of a set used to be kept in the order of the pattern, so that
/// sets with the same characters (such as `[ab]` and `[ba]`) generated
/// differently. A character which was listed more than once (as in `[aab]`)
/// was more likely, but the entropy counted it as a different character every
/// time, which overstated it. The ranges are now sorted, split where they
/// overlap, merged where they are adjacent, and know how many times their
/// characters were listed.
test_result test_regression_parse_set_weights(void) {
    const uint32_t sorted[] = {'a', 'b', 1};
    assert(set_ranges_are("[ba]", sorted, 1));
    assert(set_ranges_are("[ab]", sorted, 1));
    assert(set_ranges_are("[a-b]", sorted, 1));

    const uint32_t twice[] = {'a', 'a', 2, 'b', 'b', 1};
    assert(set_ranges_are("[aab]", twice, 2));
    assert(set_ranges_are("[aba]", twice, 2));

    const uint32_t thrice[] = {'a', 'a', 3};
    assert(set_ranges_are("[aaa]", thrice, 1));

    const uint32_t inside[] = {'a', 'b', 1, 'c', 'c', 2, 'd', 'z', 1};
    assert(set_ranges_are("[a-zc]", inside, 3));

    const uint32_t overlap[] = {'a', 'm', 2, 'n', 'z', 1};
    assert(set_ranges_are("[a-ma-z]", overlap, 2));
    assert(set_ranges_are("[a-za-m]", overlap, 2));

    const uint32_t nested[] = {'a', 'b', 1, 'c', 'd', 2, 'e', 'e', 3, 'f', 'z', 1};
    assert(set_ranges_are("[a-zc-ee]", nested, 4));

    // the last codepoint, whose range ends right before 0x110000.
    const uint32_t last[] = {'a', 'a', 1, 0x10ffff, 0x10ffff, 2};
    assert(set_ranges_are("[\\u{10ffff}a\\u{10ffff}]", last, 2));

    // with the zero source of randomness, the first choice is taken, which is
    // the smallest character. its entropy depends on how often it is listed.
    passgen_random *random = passgen_random_open(NULL, "zero");
    assert(random);
    passgen_env *env = passgen_env_new(random);
    assert(env);
    const struct {
        const char *pattern;
        const char *output;
        double entropy;
    } cases[] = {
        {"[ba]", "a", 1.0},
        {"[aab]", "a", log2(3) - log2(2)},
        {"[cbb]", "b", log2(3) - log2(2)},
        {"[a-zc]", "a", log2(27)},
        {"[aaa]", "a", 0.0},
        {NULL, NULL, 0.0},
    };
    for(size_t i = 0; cases[i].pattern; i++) {
        passgen_pattern *pattern;
        passgen_error error;
        assert(0 == passgen_parse(&pattern, &error, cases[i].pattern));
        uint8_t buffer[8] = {0};
        size_t len = 0;
        double entropy = -1;
        assert(
            0 == passgen_generate_fill_utf8(
                     pattern,
                     env,
                     NULL,
                     &entropy,
                     buffer,
                     sizeof(buffer) - 1,
                     &len));
        buffer[len] = 0;
        assert_streq((const char *) buffer, cases[i].output);
        assert(fabs(entropy - cases[i].entropy) < 0.0001);
        passgen_pattern_free(pattern);
    }
    passgen_env_free(env);
    passgen_random_free(random);

    return test_ok;
}

/// A repetition or a multiplier without any number (as in `a{}`) used to be
/// read as zero. Leaving out only the minimum (as in `a{,3}`) is fine.
test_result test_regression_parse_repeat_empty(void) {
    passgen_pattern *pattern;
    passgen_error error;

    assert(PASSGEN_ERROR_REPEAT == passgen_parse(&pattern, &error, "a{}"));
    assert(PASSGEN_ERROR_REPEAT == passgen_parse(&pattern, &error, "a{,}"));
    assert(PASSGEN_ERROR_REPEAT == passgen_parse(&pattern, &error, "{}a|b"));
    assert(PASSGEN_ERROR_REPEAT == passgen_parse(&pattern, &error, "(a|{}b)"));

    assert(0 == passgen_parse(&pattern, &error, "a{,3}"));
    passgen_pattern_segment *segment =
        passgen_pattern_group_segment_get(&pattern->group, 0);
    passgen_pattern_item *item = passgen_stack_get(&segment->items, 0);
    assert(item->repeat.min == 0);
    assert(item->repeat.max == 3);
    passgen_pattern_free(pattern);

    return test_ok;
}

/// Unicode escapes can be up to six hex digits long, which is more than what
/// is a valid codepoint. Those used to be accepted, and corrupted memory when
/// used in the name of a wordlist.
test_result test_regression_parse_unicode_escape_range(void) {
    passgen_pattern *pattern;
    passgen_error error;

    assert(0 == passgen_parse(&pattern, &error, "\\u{10ffff}"));
    passgen_pattern_free(pattern);

    assert(0 != passgen_parse(&pattern, &error, "\\u{110000}"));
    passgen_pattern_free(pattern);

    assert(0 != passgen_parse(&pattern, &error, "\\u{ffffff}"));
    passgen_pattern_free(pattern);

    assert(0 != passgen_parse(&pattern, &error, "\\w{a\\u{fc1f30}b}"));
    passgen_pattern_free(pattern);

    assert(0 == passgen_parse(&pattern, &error, "\\w{a\\u{fc}b}"));
    passgen_pattern_free(pattern);

    return test_ok;
}

/// When checking if a character has a meaning in a pattern, only the lowest
/// byte of its codepoint was looked at. Characters such as U+017C (a z with a
/// dot above), which ends in the same byte as the pipe (U+007C), were taken
/// for syntax.
test_result test_regression_parse_unicode_syntax(void) {
    passgen_pattern *pattern;
    passgen_error error;
    passgen_random *random = passgen_random_open(NULL, "zero");
    assert(random);
    passgen_env *env = passgen_env_new(random);
    assert(env);

    // one for each of the characters which have a meaning: ( ) ? [ { | }
    const char *patterns[] = {
        "a\xc4\xa8z",
        "a\xc4\xa9z",
        "a\xc4\xbfz",
        "a\xc5\x9bz",
        "a\xc5\xbbz",
        "a\xc5\xbcz",
        "a\xc5\xbdz",
        // and the same in another plane
        "a\xf0\x9f\x81\xbcz",
        NULL,
    };

    for(size_t i = 0; patterns[i]; i++) {
        // they are literals, so the pattern generates itself.
        uint8_t output[32] = {0};
        assert(0 == passgen_parse(&pattern, &error, patterns[i]));
        size_t len = 0;
        passgen_generate_fill_utf8(
            pattern,
            env,
            NULL,
            NULL,
            output,
            sizeof(output) - 1,
            &len);
        output[len] = 0;
        assert_streq((const char *) output, patterns[i]);
        passgen_pattern_free(pattern);
    }

    // escaping them does not give them a meaning either.
    assert(0 != passgen_parse(&pattern, &error, "a\\\xc5\xbcz"));
    passgen_pattern_free(pattern);

    passgen_env_free(env);
    passgen_random_free(random);

    return test_ok;
}

/// There was no way to have a question mark in the output, as it could not be
/// escaped like the other characters which have a meaning in patterns.
test_result test_regression_parse_escaped_question_mark(void) {
    passgen_pattern *pattern;
    passgen_error error;
    passgen_random *random = passgen_random_open(NULL, "zero");
    assert(random);
    passgen_env *env = passgen_env_new(random);
    assert(env);

    uint8_t output[32] = {0};
    assert(0 == passgen_parse(&pattern, &error, "why\\?\\?{2}(\\?)"));
    size_t len = 0;
    passgen_generate_fill_utf8(
        pattern,
        env,
        NULL,
        NULL,
        output,
        sizeof(output) - 1,
        &len);
    output[len] = 0;
    assert_streq((const char *) output, "why????");
    passgen_pattern_free(pattern);

    // in a set, it does not have a meaning, and is not escaped.
    assert(0 == passgen_parse(&pattern, &error, "[?]"));
    passgen_generate_fill_utf8(
        pattern,
        env,
        NULL,
        NULL,
        output,
        sizeof(output) - 1,
        &len);
    output[len] = 0;
    assert_streq((const char *) output, "?");
    passgen_pattern_free(pattern);

    // other characters still cannot be escaped.
    assert(0 != passgen_parse(&pattern, &error, "a\\.b"));
    passgen_pattern_free(pattern);

    passgen_env_free(env);
    passgen_random_free(random);

    return test_ok;
}

/// There was no way to have a closing bracket in a set. A range which ended in
/// an escaped character also kept the marker for escaped characters as part
/// of the codepoint, so it was far too large, and generating from it failed.
test_result test_regression_parse_set_escapes(void) {
    passgen_pattern *pattern;
    passgen_error error;
    passgen_random *random = passgen_random_open(NULL, "xorshift:1234");
    assert(random);
    passgen_env *env = passgen_env_new(random);
    assert(env);

    // each of these sets has exactly the characters listed.
    const char *sets[][2] = {
        {"[\\]]", "]"},
        {"[\\[\\]]", "[]"},
        {"[a\\]\\-]", "a]-"},
        {"[+-\\-]", "+,-"},
        {"[Z-\\]]", "Z[\\]"},
        {"[\\[-\\]]", "[\\]"},
        {NULL, NULL},
    };

    for(size_t i = 0; sets[i][0]; i++) {
        assert(0 == passgen_parse(&pattern, &error, sets[i][0]));

        bool seen[8] = {false};
        size_t expected_len = strlen(sets[i][1]);
        for(size_t n = 0; n < 200; n++) {
            uint8_t output[8] = {0};
            size_t len = 0;
            passgen_generate_fill_utf8(
                pattern,
                env,
                NULL,
                NULL,
                output,
                sizeof(output) - 1,
                &len);
            assert(len == 1);
            const char *found = strchr(sets[i][1], output[0]);
            assert(found);
            seen[found - sets[i][1]] = true;
        }

        for(size_t c = 0; c < expected_len; c++) {
            assert(seen[c]);
        }

        passgen_pattern_free(pattern);
    }

    // other characters cannot be escaped in a set, and a range still cannot
    // end before it starts.
    assert(0 != passgen_parse(&pattern, &error, "[\\q]"));
    passgen_pattern_free(pattern);
    assert(0 != passgen_parse(&pattern, &error, "[a-\\-]"));
    passgen_pattern_free(pattern);

    passgen_env_free(env);
    passgen_random_free(random);

    return test_ok;
}

/// An empty set used to be accepted, and generated nothing. A set stands for
/// one of its characters, so there is nothing that it could generate.
test_result test_regression_parse_set_empty(void) {
    passgen_pattern *pattern;
    passgen_error error;

    const char *patterns[] = {"[]", "a[]b", "[]{3}", "([])", "[]?", NULL};
    for(size_t i = 0; patterns[i]; i++) {
        assert(0 != passgen_parse(&pattern, &error, patterns[i]));
        passgen_pattern_free(pattern);
    }

    // brackets which are escaped are not a set.
    assert(0 == passgen_parse(&pattern, &error, "\\[\\]"));
    passgen_pattern_free(pattern);

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
    passgen_pattern *pattern;
    passgen_error error;
    passgen_random *random = passgen_random_open(NULL, "zero");
    assert(random);
    passgen_env *env = passgen_env_new(random);
    assert(env);

    double entropy = 0;
    size_t count = 0;

    assert(0 == passgen_parse(&pattern, &error, "a?"));
    assert(0 == passgen_generate(pattern, env, NULL, &entropy, &count, count_codepoints));
    assert(fabs(entropy - 1.0) < 0.0001);
    passgen_pattern_free(pattern);

    assert(0 == passgen_parse(&pattern, &error, "a?[ab]?(c|d)?"));
    assert(0 == passgen_generate(pattern, env, NULL, &entropy, &count, count_codepoints));
    // with the zero randomness source, every optional item is emitted, so
    // this is one bit for each of them, and one each for the set and the
    // group.
    assert(fabs(entropy - 5.0) < 0.0001);
    passgen_pattern_free(pattern);

    passgen_env_free(env);
    passgen_random_free(random);

    return test_ok;
}

/// The entropy used to be computed as the number of possible outputs, which
/// is too large for a double above 1024 bits: it was reported as infinite. It
/// is computed in bits now.
test_result test_regression_entropy_large(void) {
    passgen_pattern *pattern;
    passgen_error error;
    passgen_random *random = passgen_random_open(NULL, "zero");
    assert(random);
    passgen_env *env = passgen_env_new(random);
    assert(env);

    double entropy = 0;
    size_t count = 0;

    // 300 lowercase letters have about 1410 bits.
    assert(0 == passgen_parse(&pattern, &error, "[a-z]{300}"));
    assert(0 == passgen_generate(pattern, env, NULL, &entropy, &count, count_codepoints));
    assert(isfinite(entropy));
    assert(fabs(entropy - 300 * log2(26)) < 0.001);
    passgen_pattern_free(pattern);

    // something which always generates the same has none.
    assert(0 == passgen_parse(&pattern, &error, "abc(def){3}"));
    assert(0 == passgen_generate(pattern, env, NULL, &entropy, &count, count_codepoints));
    assert(entropy == 0.0);
    passgen_pattern_free(pattern);

    // a segment which is more likely to be chosen has less. with the zero
    // source of randomness, the first one is chosen, which is three of four.
    assert(0 == passgen_parse(&pattern, &error, "{3}a|b"));
    assert(0 == passgen_generate(pattern, env, NULL, &entropy, &count, count_codepoints));
    assert(fabs(entropy - (log2(4) - log2(3))) < 0.0001);
    passgen_pattern_free(pattern);

    passgen_env_free(env);
    passgen_random_free(random);

    return test_ok;
}

/// The JSON output must escape all control characters.
test_result test_regression_json_control_chars(void) {
    passgen_pattern *pattern;
    passgen_error error;
    passgen_random *random = passgen_random_open(NULL, "zero");
    assert(random);
    passgen_env *env = passgen_env_new(random);
    assert(env);

    // literal control character U+0001 and a tab, a quote and a backslash
    assert(0 == passgen_parse(&pattern, &error, "\x01\t\"[\\\\]"));

    uint8_t buffer[64] = {0};
    size_t len = 0;
    passgen_generate_fill_json_utf8(
        pattern,
        env,
        NULL,
        NULL,
        buffer,
        sizeof(buffer) - 1,
        &len);
    buffer[len] = 0;
    assert_streq((const char *) buffer, "\\u0001\\t\\\"\\\\");

    passgen_pattern_free(pattern);
    passgen_env_free(env);
    passgen_random_free(random);

    return test_ok;
}

#ifndef _WIN32 // fmemopen does not exist on windows

/// Sets up an environment with a single wordlist named `list`. The source of
/// randomness is not owned by the environment, so it is returned as well, for
/// the caller to free.
static passgen_env *wordlist_env(
    passgen_random **random,
    const char *random_desc,
    char *data,
    size_t len,
    size_t level) {
    // fmemopen() with an empty buffer is not portable, so use an empty file
    // in that case.
    FILE *file = len ? fmemopen(data, len, "r") : tmpfile();
    if(!file) {
        return NULL;
    }

    *random = passgen_random_open(NULL, random_desc);
    passgen_env *env = *random ? passgen_env_new(*random) : NULL;
    if(!env) {
        fclose(file);
        if(*random) {
            passgen_random_free(*random);
        }
        return NULL;
    }

    // this closes the file if it fails.
    if(0 != passgen_env_wordlist_add(env, "list", file, level)) {
        passgen_env_free(env);
        passgen_random_free(*random);
        return NULL;
    }

    return env;
}

/// Picking from an empty wordlist used to hang (release builds) or trip an
/// assertion (debug builds). It should just be an error.
test_result test_regression_wordlist_empty(void) {
    passgen_pattern *pattern;
    passgen_error error;
    passgen_random *random;
    passgen_env *env;
    size_t count = 0;

    env = wordlist_env(&random, "zero", NULL, 0, 3);
    assert(env);
    assert(0 == passgen_parse(&pattern, &error, "\\w{list}"));
    assert(0 != passgen_generate(pattern, env, NULL, NULL, &count, count_codepoints));
    passgen_pattern_free(pattern);

    assert(0 == passgen_parse(&pattern, &error, "\\m{list}"));
    assert(0 != passgen_generate(pattern, env, NULL, NULL, &count, count_codepoints));
    passgen_pattern_free(pattern);
    passgen_env_free(env);
    passgen_random_free(random);

    // wordlist that does not exist at all
    random = passgen_random_open(NULL, "zero");
    assert(random);
    env = passgen_env_new(random);
    assert(env);
    assert(0 == passgen_parse(&pattern, &error, "\\w{nope}\\m{nope}"));
    assert(0 != passgen_generate(pattern, env, NULL, NULL, &count, count_codepoints));
    passgen_pattern_free(pattern);
    passgen_env_free(env);
    passgen_random_free(random);

    return test_ok;
}

/// A wordlist without a name (as in `\w{}`) used to be looked up using a null
/// pointer, which crashed as soon as any wordlist was defined. It is not a
/// valid pattern now.
test_result test_regression_wordlist_no_name(void) {
    passgen_pattern *pattern;

    assert(PASSGEN_ERROR_SPECIAL == passgen_parse(&pattern, NULL, "\\w{}"));
    assert(PASSGEN_ERROR_SPECIAL == passgen_parse(&pattern, NULL, "\\m{}"));
    assert(PASSGEN_ERROR_SPECIAL == passgen_parse(&pattern, NULL, "a\\w{}b"));

    assert(PASSGEN_OK == passgen_parse(&pattern, NULL, "\\w{a}"));
    passgen_pattern_free(pattern);

    return test_ok;
}

/// Words longer than 128 codepoints used to fail (wordlist) or overflow a
/// stack buffer (markov chain).
test_result test_regression_wordlist_long_words(void) {
    passgen_pattern *pattern;
    passgen_error error;
    passgen_random *random;
    passgen_env *env;

    // a single word consisting of 600 times the letter a.
    size_t word_len = 600;
    char *data = malloc(word_len + 1);
    memset(data, 'a', word_len);
    data[word_len] = '\n';

    env = wordlist_env(&random, "xorshift:1234", data, word_len + 1, 1);
    assert(env);

    size_t count = 0;
    assert(0 == passgen_parse(&pattern, &error, "\\w{list}"));
    assert(0 == passgen_generate(pattern, env, NULL, NULL, &count, count_codepoints));
    assert_eq(count, word_len);
    passgen_pattern_free(pattern);

    // at every step, the markov chain has a 599/600 chance of generating
    // another a, so these words get long: on average 600 codepoints.
    count = 0;
    double entropy = 0;
    assert(0 == passgen_parse(&pattern, &error, "\\m{list}{100}"));
    assert(
        0 ==
        passgen_generate(pattern, env, NULL, &entropy, &count, count_codepoints));
    assert(count > 128 * 100);
    assert(entropy > 0.0);
    passgen_pattern_free(pattern);

    passgen_env_free(env);
    passgen_random_free(random);
    free(data);

    return test_ok;
}

/// Defining a wordlist twice replaces the first one (and must not leak it).
test_result test_regression_wordlist_redefine(void) {
    passgen_pattern *pattern;
    passgen_error error;
    passgen_random *random;
    passgen_env *env;
    char first[] = "first\n";
    char second[] = "second\n";

    env = wordlist_env(&random, "zero", first, strlen(first), 3);
    assert(env);

    FILE *file = fmemopen(second, strlen(second), "r");
    assert(file);
    assert(0 == passgen_env_wordlist_add(env, "list", file, 3));

    uint8_t buffer[32] = {0};
    assert(0 == passgen_parse(&pattern, &error, "\\w{list}"));
    size_t len = 0;
    passgen_generate_fill_utf8(
        pattern,
        env,
        NULL,
        NULL,
        buffer,
        sizeof(buffer) - 1,
        &len);
    buffer[len] = 0;
    assert_streq((const char *) buffer, "second");

    passgen_pattern_free(pattern);
    passgen_env_free(env);
    passgen_random_free(random);

    return test_ok;
}

#endif
