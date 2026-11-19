#include "passgen/generate.h"
#include "passgen/container/stack.h"
#include "passgen/parser/parser.h"
#include "passgen/parser/token.h"
#include "passgen/passgen.h"
#include "passgen/pattern/group.h"
#include "passgen/pattern/literal.h"
#include "passgen/pattern/pattern.h"
#include "passgen/pattern/range.h"
#include "passgen/pattern/repeat.h"
#include "passgen/pattern/segment.h"
#include "passgen/pattern/segment_item.h"
#include "passgen/pattern/set.h"
#include "stats.h"
#include "tests.h"
#include <string.h>

// don't change this seed - that will break tests!
#define SEED 320843200

#define PREAMBLE()                                       \
    passgen_parser parser;                               \
    passgen_token_parser token_parser = {0};             \
    passgen_token token = {0};                           \
    passgen_env *env;                                    \
    passgen_pattern parsed_pattern;                      \
    const char *pattern;                                 \
    passgen_random random;                               \
    assert(passgen_random_xorshift_open(&random, SEED)); \
    env = passgen_env_new(&random);                      \
    assert(env);

#define GENERATE(output, pattern)                                          \
    do {                                                                   \
        passgen_parser_init(&parser, &parsed_pattern);                     \
        passgen_token_parser_init(&token_parser);                          \
                                                                           \
        int token_parser_state = PASSGEN_TOKEN_INIT;                       \
        for(size_t i = 0; pattern[i]; i++) {                               \
            token_parser_state =                                           \
                passgen_token_parse(&token_parser, &token, 1, pattern[i]); \
            if(token_parser_state == PASSGEN_TOKEN_INIT) {                 \
                assert(0 == passgen_parse_token(&parser, &token));         \
            }                                                              \
        }                                                                  \
                                                                           \
        assert(token_parser_state == PASSGEN_TOKEN_INIT);                  \
        assert(0 == passgen_parse_finish(&parser));                        \
        assert(0 == passgen_resolve(parser.pattern, env, NULL));           \
        size_t len = 0;                                                    \
        passgen_generate_fill_unicode(                                     \
            parser.pattern,                                                \
            env,                                                           \
            NULL,                                                          \
            NULL,                                                          \
            output,                                                        \
            sizeof(output),                                                \
            &len);                                                         \
        output[len] = 0;                                                   \
                                                                           \
        passgen_parser_free(&parser);                                      \
        passgen_pattern_deinit(&parsed_pattern);                           \
    } while(0)

#define POSTAMBLE()        \
    passgen_env_free(env); \
    passgen_random_close(&random)

test_result test_generate_empty(void) {
    PREAMBLE();
    uint32_t output[10];

    pattern = "";
    GENERATE(output, pattern);
    assert(output[0] == '\0');

    POSTAMBLE();
    return test_ok;
}

test_result test_generate_chars(void) {
    PREAMBLE();
    uint32_t output[10];

    // single char
    pattern = "a";
    GENERATE(output, pattern);
    assert(output[0] == 'a');
    assert(output[1] == '\0');

    // multiple chars
    pattern = "abc";
    GENERATE(output, pattern);
    assert(output[0] == 'a');
    assert(output[1] == 'b');
    assert(output[2] == 'c');
    assert(output[3] == '\0');

    // escaped chars
    pattern = "pass\\[\\{\\u{78}";
    GENERATE(output, pattern);
    assert(output[0] == 'p');
    assert(output[1] == 'a');
    assert(output[2] == 's');
    assert(output[3] == 's');
    assert(output[4] == '[');
    assert(output[5] == '{');
    assert(output[6] == 'x');
    assert(output[7] == '\0');

    POSTAMBLE();
    return test_ok;
}

/// Checks what a pattern generates: every output has to be one of the expected
/// ones (the list is terminated by NULL), and every expected output has to
/// come up at some point. This does not depend on the exact sequence of random
/// numbers, unlike comparing against the output for a specific seed.
static bool
generates_exactly(const char *pattern_string, const char *const *expected) {
    passgen_pattern *pattern;
    passgen_error error;
    if(0 != passgen_parse(&pattern, &error, pattern_string)) {
        passgen_pattern_free(pattern);
        return false;
    }

    size_t expected_len = 0;
    while(expected[expected_len]) {
        expected_len++;
    }

    passgen_random random;
    if(!passgen_random_xorshift_open(&random, SEED)) {
        passgen_pattern_free(pattern);
        return false;
    }

    passgen_env *env = passgen_env_new(&random);
    if(!env) {
        passgen_random_close(&random);
        passgen_pattern_free(pattern);
        return false;
    }

    bool seen[64] = {false};
    size_t seen_count = 0;
    bool okay = expected_len <= 64;
    okay = okay && PASSGEN_OK == passgen_resolve(pattern, env, NULL);

    // with the most outputs we test for (27 of them), the chance of not
    // seeing all of them in this many tries is negligible. and as the seed is
    // fixed, this is deterministic anyways.
    for(size_t i = 0; okay && seen_count < expected_len && i < 10000; i++) {
        uint8_t output[64];
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

        okay = false;
        for(size_t e = 0; e < expected_len; e++) {
            if(0 == strcmp((const char *) output, expected[e])) {
                okay = true;
                if(!seen[e]) {
                    seen[e] = true;
                    seen_count++;
                }
                break;
            }
        }
    }

    passgen_env_free(env);
    passgen_random_close(&random);
    passgen_pattern_free(pattern);

    return okay && seen_count == expected_len;
}

#define EXPECTED(...) ((const char *const[]) {__VA_ARGS__, NULL})

/// Whether a pattern generates each of the outputs (the list is terminated by
/// NULL) as often as their weights say, and nothing else.
static bool generates_weighted(
    const char *pattern_string,
    const char *const *outputs,
    const double *weights) {
    passgen_pattern *pattern;
    if(0 != passgen_parse(&pattern, NULL, pattern_string)) {
        return false;
    }

    size_t outputs_len = 0;
    while(outputs[outputs_len]) {
        outputs_len++;
    }

    passgen_random random;
    if(!passgen_random_xorshift_open(&random, SEED)) {
        passgen_pattern_free(pattern);
        return false;
    }
    passgen_env *env = passgen_env_new(&random);

    uint64_t counts[16] = {0};
    bool okay = env && outputs_len >= 2 && outputs_len <= 16;
    okay = okay && PASSGEN_OK == passgen_resolve(pattern, env, NULL);
    for(size_t i = 0; okay && i < 60000; i++) {
        uint8_t output[64];
        size_t len = 0;
        okay = PASSGEN_OK == passgen_generate_fill_utf8(
                                 pattern,
                                 env,
                                 NULL,
                                 NULL,
                                 output,
                                 sizeof(output) - 1,
                                 &len);
        output[len] = 0;

        // anything which is not one of the outputs fails.
        size_t found = outputs_len;
        for(size_t o = 0; okay && o < outputs_len; o++) {
            if(0 == strcmp((const char *) output, outputs[o])) {
                found = o;
            }
        }
        okay = okay && found < outputs_len;
        if(okay) {
            counts[found] += 1;
        }
    }

    passgen_env_free(env);
    passgen_random_close(&random);
    passgen_pattern_free(pattern);

    return okay && chi_squared_fits(outputs_len, counts, weights);
}

#define WEIGHTS(...) ((const double[]) {__VA_ARGS__})

/// What is generated comes up as often as the pattern says: characters which
/// are listed more than once in a set, choices with multipliers, optional
/// items and ranges of repetitions.
test_result test_generate_distribution(void) {
    assert(generates_weighted("[ab]", EXPECTED("a", "b"), WEIGHTS(1, 1)));
    assert(generates_weighted("[aab]", EXPECTED("a", "b"), WEIGHTS(2, 1)));
    assert(generates_weighted("[ba]", EXPECTED("a", "b"), WEIGHTS(1, 1)));
    assert(generates_weighted(
        "[a-cb]",
        EXPECTED("a", "b", "c"),
        WEIGHTS(1, 2, 1)));
    assert(generates_weighted(
        "(a|b|c)",
        EXPECTED("a", "b", "c"),
        WEIGHTS(1, 1, 1)));
    assert(generates_weighted("{3}a|b", EXPECTED("a", "b"), WEIGHTS(3, 1)));
    assert(generates_weighted("a?", EXPECTED("a", ""), WEIGHTS(1, 1)));
    assert(generates_weighted(
        "a{0,3}",
        EXPECTED("", "a", "aa", "aaa"),
        WEIGHTS(1, 1, 1, 1)));
    // half of the time nothing, and the rest split two to one.
    assert(generates_weighted(
        "({2}a|b)?",
        EXPECTED("", "a", "b"),
        WEIGHTS(3, 2, 1)));

    // the check notices when something is off.
    assert(!generates_weighted("[aab]", EXPECTED("a", "b"), WEIGHTS(1, 1)));
    assert(!generates_weighted("[ab]", EXPECTED("a"), WEIGHTS(1, 1)));

    return test_ok;
}

test_result test_generate_segments(void) {
    // either a or b
    assert(generates_exactly("a|b", EXPECTED("a", "b")));
    assert(generates_exactly("a|b|c", EXPECTED("a", "b", "c")));
    assert(generates_exactly("ab|cd", EXPECTED("ab", "cd")));

    // the helper notices unexpected and missing outputs
    assert(!generates_exactly("a|b", EXPECTED("a")));
    assert(!generates_exactly("a|b", EXPECTED("a", "b", "c")));

    return test_ok;
}

test_result test_generate_group(void) {
    assert(generates_exactly("(abc)", EXPECTED("abc")));
    assert(generates_exactly("(a|b)", EXPECTED("a", "b")));
    assert(generates_exactly("(a|b)c", EXPECTED("ac", "bc")));
    assert(generates_exactly("((a|b)|c)", EXPECTED("a", "b", "c")));

    // empty groups generate nothing, also when they are repeated or optional,
    // which does not apply to what comes before them.
    assert(generates_exactly("()", EXPECTED("")));
    assert(generates_exactly("a(){3}", EXPECTED("a")));
    assert(generates_exactly("a(){3}b", EXPECTED("ab")));
    assert(generates_exactly("a(|){1,3}", EXPECTED("a")));
    assert(generates_exactly("a()?", EXPECTED("a")));
    assert(generates_exactly("x|(){0}y", EXPECTED("x", "y")));

    return test_ok;
}

test_result test_generate_set(void) {
    assert(generates_exactly("[a]", EXPECTED("a")));
    assert(generates_exactly("[abc]", EXPECTED("a", "b", "c")));
    assert(generates_exactly("[a-c]", EXPECTED("a", "b", "c")));
    assert(generates_exactly("[a-cx]", EXPECTED("a", "b", "c", "x")));

    // sets with a number of choices that is a power of two
    assert(generates_exactly("[ab]", EXPECTED("a", "b")));
    assert(generates_exactly("[a-d]", EXPECTED("a", "b", "c", "d")));

    return test_ok;
}

test_result test_generate_maybe(void) {
    // char or not?
    assert(generates_exactly("a?", EXPECTED("a", "")));

    // group or not?
    assert(generates_exactly("(b)?", EXPECTED("b", "")));

    // set or not?
    assert(generates_exactly("[c]?", EXPECTED("c", "")));

    assert(generates_exactly("a?b", EXPECTED("ab", "b")));

    return test_ok;
}

test_result test_generate_repeat(void) {
    assert(generates_exactly("a{5}", EXPECTED("aaaaa")));
    assert(generates_exactly("a{0}", EXPECTED("")));
    assert(generates_exactly("b{2,3}", EXPECTED("bb", "bbb")));
    assert(generates_exactly("b{0,2}", EXPECTED("", "b", "bb")));

    assert(generates_exactly(
        "[ab]{3}",
        EXPECTED("aaa", "aab", "aba", "abb", "baa", "bab", "bba", "bbb")));

    assert(generates_exactly(
        "(ab|x){3}",
        EXPECTED(
            "ababab",
            "ababx",
            "abxab",
            "abxx",
            "xabab",
            "xabx",
            "xxab",
            "xxx")));

    // all 27 combinations
    char combinations[27][4];
    const char *expected[28];
    for(size_t i = 0; i < 27; i++) {
        combinations[i][0] = 'a' + (i / 9) % 3;
        combinations[i][1] = 'a' + (i / 3) % 3;
        combinations[i][2] = 'a' + i % 3;
        combinations[i][3] = 0;
        expected[i] = combinations[i];
    }
    expected[27] = NULL;
    assert(generates_exactly("[abc]{3}", expected));

    return test_ok;
}

static int dummy_callback(void *data, uint32_t codepoint) {
    (void) data;
    (void) codepoint;
    return 0;
}

// Generate from the pattern with the given step limit, or the default one if
// it is zero, and return the status.
static int generate_steps(const char *string, size_t step_limit) {
    passgen_pattern *pattern;
    passgen_error error;
    if(0 != passgen_parse(&pattern, &error, string)) {
        return -1;
    }

    passgen_random random;
    passgen_random_open(&random, "xorshift:1234");

    passgen_env *env = passgen_env_new(&random);
    if(step_limit) {
        passgen_env_step_limit_set(env, step_limit);
    }

    int ret = passgen_resolve(pattern, env, NULL);
    if(ret == PASSGEN_OK) {
        ret = passgen_generate(pattern, env, NULL, NULL, NULL, dummy_callback);
    }

    passgen_pattern_free(pattern);
    passgen_env_free(env);
    passgen_random_close(&random);

    return ret;
}

test_result test_generate_step_limit(void) {
    // every character of a set is a step.
    assert_eq(generate_steps("[a-z]{20}", 20), PASSGEN_OK);
    assert_eq(generate_steps("[a-z]{20}", 19), PASSGEN_ERROR_STEP_LIMIT);

    // as is every repetition of a group, and what is in it.
    assert_eq(generate_steps("([ab]){3}", 6), PASSGEN_OK);
    assert_eq(generate_steps("([ab]){3}", 5), PASSGEN_ERROR_STEP_LIMIT);

    // a run of literal characters is one step.
    assert_eq(generate_steps("abc", 1), PASSGEN_OK);
    assert_eq(generate_steps("abc{2}", 2), PASSGEN_ERROR_STEP_LIMIT);

    // what is left out does not take a step.
    assert_eq(generate_steps("[ab]{0}", 1), PASSGEN_OK);

    // the pattern itself does not count, so nothing takes no steps.
    assert_eq(generate_steps("", 1), PASSGEN_OK);

    return test_ok;
}

test_result test_generate_step_limit_default(void) {
    // the default is a million steps.
    assert_eq(generate_steps("(a{1000}){999}", 0), PASSGEN_OK);
    assert_eq(generate_steps("(a{1000}){1000}", 0), PASSGEN_ERROR_STEP_LIMIT);

    // which is what keeps this from running practically forever: it does
    // not generate anything, but every repetition is a step.
    assert_eq(generate_steps("(){999999999}", 0), PASSGEN_ERROR_STEP_LIMIT);
    assert_eq(
        generate_steps("((((){999}){999}){999}){999}", 0),
        PASSGEN_ERROR_STEP_LIMIT);

    return test_ok;
}

#undef SEED
#undef PREAMBLE
#undef POSTAMBLE

/// Generates from the pattern with xorshift, and returns the next random
/// number after it, to find out how much randomness it took. The pattern can
/// use the words, as the wordlist `words`, if they are not NULL.
static uint64_t
random_after_words(const char *string, const char *words, char *output) {
    passgen_random random;
    passgen_random_open(&random, "xorshift:1234");
    passgen_env *env = passgen_env_new(&random);
    output[0] = 0;
    if(env && words) {
        FILE *file = tmpfile();
        if(!file) {
            passgen_env_free(env);
            passgen_random_close(&random);
            return 0;
        }
        fputs(words, file);
        passgen_env_wordlist_add(env, "words", file);
    }
    passgen_pattern *pattern;
    passgen_error error;
    uint64_t next = 0;
    if(env && 0 == passgen_parse(&pattern, &error, string)) {
        size_t length = 0;
        if(0 == passgen_resolve(pattern, env, NULL)) {
            passgen_generate_fill_utf8(
                pattern,
                env,
                NULL,
                NULL,
                (uint8_t *) output,
                63,
                &length);
        }
        output[length] = 0;
        passgen_pattern_free(pattern);
        next = passgen_random_u64(&random);
    }
    passgen_env_free(env);
    passgen_random_close(&random);
    return next;
}

static uint64_t random_after(const char *string, char *output) {
    return random_after_words(string, NULL, output);
}

/// Groups with a single segment and sets of a single character have nothing
/// to choose from, so they take no randomness, like a repetition with a fixed
/// count. So `(abc)` generates the same as `abc`.
test_result test_generate_single_choice(void) {
    char output[64];
    uint64_t untouched = random_after("", output);
    assert_streq(output, "");

    const char *patterns[] = {
        "abc",
        "(abc)",
        "({2}abc)",
        "((a)(b)){2}(c)",
        "[a][bb][c-c]",
        "a{3}",
        "()",
    };
    const char *expected[] = {"abc", "abc", "abc", "ababc", "abc", "aaa", ""};
    for(size_t i = 0; i < sizeof(patterns) / sizeof(patterns[0]); i++) {
        assert_eq(random_after(patterns[i], output), untouched);
        assert_streq(output, expected[i]);
    }

    // something to choose from takes randomness, also when the choices are
    // equal.
    const char *choices[] = {"(a|a)", "[ab]", "a{1,2}", "a?", "(|)"};
    for(size_t i = 0; i < sizeof(choices) / sizeof(choices[0]); i++) {
        assert_ne(random_after(choices[i], output), untouched);
    }

    // so these generate the same, from the same randomness.
    char grouped[64];
    random_after("([a-z]{8})-(x)[0-9]", grouped);
    random_after("[a-z]{8}-x[0-9]", output);
    assert_streq(grouped, output);

    return test_ok;
}

/// The same goes for wordlists with a single word, and for Markov chains
/// where only one codepoint can follow, which is all of them for a wordlist
/// of a single word.
test_result test_generate_single_word(void) {
    char output[64];
    uint64_t untouched = random_after("", output);

    assert_eq(random_after_words("\\w{words}", "only\n", output), untouched);
    assert_streq(output, "only");
    assert_eq(random_after_words("\\m{words}", "only\n", output), untouched);
    assert_streq(output, "only");

    // with more than one, there is something to choose.
    assert_ne(
        random_after_words("\\w{words}", "one\ntwo\n", output),
        untouched);
    assert_ne(random_after_words("\\m{words}", "ab\nac\n", output), untouched);

    // the entropy of what has only one option is zero.
    passgen_random random;
    passgen_random_open(&random, "xorshift:1234");
    passgen_env *env = passgen_env_new(&random);
    assert(env);
    FILE *file = tmpfile();
    assert(file);
    fputs("only\n", file);
    assert(0 == passgen_env_wordlist_add(env, "words", file));
    passgen_pattern *pattern;
    passgen_error error;
    assert(0 == passgen_parse(&pattern, &error, "\\w{words}\\m{words}"));
    assert(0 == passgen_resolve(pattern, env, NULL));
    double entropy = -1;
    uint8_t buffer[64];
    assert(
        0 == passgen_generate_fill_utf8(
                 pattern,
                 env,
                 NULL,
                 &entropy,
                 buffer,
                 63,
                 NULL));
    assert(entropy == 0);
    passgen_pattern_free(pattern);
    passgen_env_free(env);
    passgen_random_close(&random);

    return test_ok;
}
