#include "tests.h"
#include <passgen/env.h>
#include <passgen/error.h>
#include <passgen/generate.h>
#include <passgen/passgen.h>
#include <passgen/random.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/// Parses, resolves and generates from a pattern with xorshift and the seed,
/// into the output (which has room for 256 bytes). Returns the status.
static passgen_status generate_seeded(
    passgen_env *env,
    const char *string,
    uint64_t seed,
    char *output) {
    char description[64];
    snprintf(description, sizeof(description), "xorshift:%llu", (unsigned long long) seed);
    passgen_random random;
    if(!passgen_random_open(&random, description)) {
        return PASSGEN_ERROR_INTERNAL;
    }
    passgen_env_random_set(env, &random);

    passgen_pattern *pattern;
    passgen_status status = passgen_parse(&pattern, NULL, string);
    if(status == PASSGEN_OK) {
        status = passgen_resolve(pattern, env, NULL);
    }
    size_t length = 0;
    if(status == PASSGEN_OK) {
        status = passgen_generate_fill_utf8(
            pattern,
            env,
            NULL,
            NULL,
            (uint8_t *) output,
            255,
            &length);
    }
    output[length] = 0;

    passgen_pattern_free(pattern);
    passgen_env_random_set(env, NULL);
    passgen_random_close(&random);
    return status;
}

static int count_presets(void *user, const char *name, const char *pattern) {
    (void) name;
    (void) pattern;
    size_t *count = user;
    *count += 1;
    return 0;
}

/// Presets are kept by name, and can be replaced.
test_result test_preset_env(void) {
    passgen_env *env = passgen_env_new(NULL);
    assert(env);

    assert(passgen_env_preset_get(env, "a") == NULL);
    assert(PASSGEN_OK == passgen_env_preset_add(env, "a", "[ab]"));
    assert(PASSGEN_OK == passgen_env_preset_add(env, ".b", "c"));
    assert_streq(passgen_env_preset_get(env, "a"), "[ab]");
    assert_streq(passgen_env_preset_get(env, ".b"), "c");

    // replacing one
    assert(PASSGEN_OK == passgen_env_preset_add(env, "a", "x"));
    assert_streq(passgen_env_preset_get(env, "a"), "x");

    size_t count = 0;
    assert(0 == passgen_env_preset_foreach(env, &count, count_presets));
    assert_eq(count, 2);

    passgen_env_free(env);
    return test_ok;
}

/// A preset generates the same as its pattern, on its own or in parentheses,
/// also when presets use other presets.
test_result test_preset_generate(void) {
    passgen_env *env = passgen_env_new(NULL);
    assert(env);
    assert(PASSGEN_OK == passgen_env_preset_add(env, "_segment", "[a-zA-Z0-9]{6}"));
    assert(
        PASSGEN_OK == passgen_env_preset_add(
                          env,
                          "apple",
                          "(\\p{_segment}-){2}\\p{_segment}"));
    assert(PASSGEN_OK == passgen_env_preset_add(env, "choice", "a|b|c"));

    const char *cases[][2] = {
        {"\\p{_segment}", "([a-zA-Z0-9]{6})"},
        {"\\p{apple}",
         "((([a-zA-Z0-9]{6})-){2}([a-zA-Z0-9]{6}))"},
        // the same as using the preset with `--preset`.
        {"\\p{apple}", "([a-zA-Z0-9]{6}-){2}[a-zA-Z0-9]{6}"},
        {"\\p{choice}", "a|b|c"},
        {"x\\p{apple}{2}y",
         "x((([a-zA-Z0-9]{6})-){2}([a-zA-Z0-9]{6})){2}y"},
        // the choice stays inside of the preset.
        {"x\\p{choice}y", "x(a|b|c)y"},
        {"\\p{choice}?", "(a|b|c)?"},
    };

    for(size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        for(uint64_t seed = 1; seed < 20; seed++) {
            char with[256], without[256];
            assert_eq(PASSGEN_OK, generate_seeded(env, cases[i][0], seed, with));
            assert_eq(PASSGEN_OK, generate_seeded(env, cases[i][1], seed, without));
            assert_streq(with, without);
        }
    }

    passgen_env_free(env);
    return test_ok;
}

/// Problems with a preset say where it is used, and what is wrong with it,
/// as the cause.
test_result test_preset_errors(void) {
    passgen_env *env = passgen_env_new(NULL);
    assert(env);
    passgen_pattern *pattern;
    passgen_error error;

    // not there
    assert(PASSGEN_OK == passgen_parse(&pattern, NULL, "ab\\p{missing}"));
    assert_eq(PASSGEN_ERROR_PRESET_NOT_FOUND, passgen_resolve(pattern, env, &error));
    assert_streq(error.detail, "missing");
    assert(error.has_position);
    assert_eq(error.start.codepoint, 2);
    assert_eq(error.end.codepoint, 13);
    passgen_error_free(&error);
    passgen_pattern_free(pattern);

    // its pattern is not valid, which the cause points to.
    assert(PASSGEN_OK == passgen_env_preset_add(env, "bad", "ab[cd"));
    assert(PASSGEN_OK == passgen_parse(&pattern, NULL, "x\\p{bad}"));
    assert_eq(PASSGEN_ERROR_PRESET_INVALID, passgen_resolve(pattern, env, &error));
    assert_streq(error.detail, "bad");
    assert_eq(error.start.codepoint, 1);
    assert_eq(error.end.codepoint, 8);
    assert(error.cause);
    assert_eq(error.cause->kind, PASSGEN_ERROR_SET_UNCLOSED);
    assert(error.cause->has_position);
    assert_eq(error.cause->start.codepoint, 5);
    passgen_error_free(&error);

    // the pattern cannot be generated then.
    assert_eq(
        PASSGEN_ERROR_INVALID_ARGUMENT,
        passgen_generate(pattern, env, NULL, NULL, NULL, NULL));
    passgen_pattern_free(pattern);

    // something which it uses, through another preset, is not there.
    assert(PASSGEN_OK == passgen_env_preset_add(env, "outer", "\\p{inner}"));
    assert(PASSGEN_OK == passgen_env_preset_add(env, "inner", "a\\w{words}"));
    assert(PASSGEN_OK == passgen_parse(&pattern, NULL, "\\p{outer}"));
    assert_eq(PASSGEN_ERROR_PRESET_INVALID, passgen_resolve(pattern, env, &error));
    assert_streq(error.detail, "outer");
    assert(error.cause);
    assert_eq(error.cause->kind, PASSGEN_ERROR_PRESET_INVALID);
    assert_streq(error.cause->detail, "inner");
    assert(error.cause->cause);
    assert_eq(error.cause->cause->kind, PASSGEN_ERROR_WORDLIST_NOT_FOUND);
    assert_streq(error.cause->cause->detail, "words");
    assert_eq(error.cause->cause->start.codepoint, 1);
    passgen_error_free(&error);

    // the error is not needed to find out what went wrong.
    assert_eq(PASSGEN_ERROR_PRESET_INVALID, passgen_resolve(pattern, env, NULL));

    // once it is there, it works. adding the wordlist means that the pattern
    // has to be resolved again.
    FILE *file = tmpfile();
    assert(file);
    fputs("word\n", file);
    assert(PASSGEN_OK == passgen_env_wordlist_add(env, "words", file));
    assert_eq(PASSGEN_OK, passgen_resolve(pattern, env, NULL));
    passgen_pattern_free(pattern);

    // presets which are not used are not parsed, so a broken one does not
    // matter to others.
    char output[256];
    assert_eq(PASSGEN_OK, generate_seeded(env, "\\p{outer}", 1, output));
    assert_streq(output, "aword");

    passgen_env_free(env);
    return test_ok;
}

/// A preset which uses itself, directly or through others, would never end.
test_result test_preset_cycle(void) {
    passgen_env *env = passgen_env_new(NULL);
    assert(env);
    assert(PASSGEN_OK == passgen_env_preset_add(env, "self", "a\\p{self}?"));
    assert(PASSGEN_OK == passgen_env_preset_add(env, "ping", "(\\p{pong}|x)"));
    assert(PASSGEN_OK == passgen_env_preset_add(env, "pong", "\\p{ping}"));

    passgen_pattern *pattern;
    passgen_error error;

    assert(PASSGEN_OK == passgen_parse(&pattern, NULL, "\\p{self}"));
    assert_eq(PASSGEN_ERROR_PRESET_INVALID, passgen_resolve(pattern, env, &error));
    assert(error.cause);
    assert_eq(error.cause->kind, PASSGEN_ERROR_PRESET_CYCLE);
    assert_streq(error.cause->detail, "self");
    assert_eq(error.cause->start.codepoint, 1);
    passgen_error_free(&error);
    passgen_pattern_free(pattern);

    assert(PASSGEN_OK == passgen_parse(&pattern, NULL, "\\p{ping}"));
    assert_eq(PASSGEN_ERROR_PRESET_INVALID, passgen_resolve(pattern, env, &error));
    assert_streq(error.detail, "ping");
    assert_eq(error.cause->kind, PASSGEN_ERROR_PRESET_INVALID);
    assert_streq(error.cause->detail, "pong");
    assert_eq(error.cause->cause->kind, PASSGEN_ERROR_PRESET_CYCLE);
    assert_streq(error.cause->cause->detail, "ping");
    passgen_error_free(&error);

    // it stays that way, nothing is left behind from the last attempt.
    assert_eq(PASSGEN_ERROR_PRESET_INVALID, passgen_resolve(pattern, env, NULL));
    passgen_pattern_free(pattern);

    // using a preset more than once is not a cycle.
    assert(PASSGEN_OK == passgen_env_preset_add(env, "x", "[xy]"));
    assert(PASSGEN_OK == passgen_env_preset_add(env, "twice", "\\p{x}\\p{x}"));
    char output[256];
    assert_eq(PASSGEN_OK, generate_seeded(env, "\\p{twice}-\\p{x}", 1, output));
    assert_eq(strlen(output), 4);

    passgen_env_free(env);
    return test_ok;
}

/// Writes a pattern of the given number of nested groups around the inner
/// part.
static char *nested(size_t count, const char *inner) {
    size_t length = 2 * count + strlen(inner);
    char *string = malloc(length + 1);
    if(!string) {
        return NULL;
    }
    memset(string, '(', count);
    strcpy(&string[count], inner);
    memset(&string[count + strlen(inner)], ')', count);
    string[length] = 0;
    return string;
}

/// Groups can be nested 1024 deep in a pattern, which is what keeps
/// generating from it from running out of stack. This is the case through
/// presets as well, where each preset counts as a group.
test_result test_preset_depth(void) {
    passgen_env *env = passgen_env_new(NULL);
    assert(env);
    char output[256];

    // the pattern, a preset in it, and 1023 groups in that are as deep as it
    // can go.
    char *deepest = nested(1023, "a");
    char *too_deep = nested(1024, "a");
    assert(deepest && too_deep);
    assert(PASSGEN_OK == passgen_env_preset_add(env, "deepest", deepest));
    assert(PASSGEN_OK == passgen_env_preset_add(env, "too-deep", too_deep));
    free(deepest);
    free(too_deep);

    assert_eq(PASSGEN_OK, generate_seeded(env, "\\p{deepest}", 1, output));
    assert_streq(output, "a");
    // also when it is used again, once it is resolved.
    assert_eq(PASSGEN_OK, generate_seeded(env, "\\p{deepest}\\p{deepest}", 1, output));
    assert_eq(
        PASSGEN_ERROR_DEPTH_LIMIT,
        generate_seeded(env, "(\\p{deepest})", 1, output));

    // the preset on its own is fine, it is only too deep where it is used.
    passgen_pattern *pattern;
    passgen_error error;
    assert(PASSGEN_OK == passgen_parse(&pattern, NULL, "ab\\p{too-deep}"));
    assert_eq(PASSGEN_ERROR_DEPTH_LIMIT, passgen_resolve(pattern, env, &error));
    assert_streq(error.detail, "too-deep");
    assert_eq(error.start.codepoint, 2);
    assert(!error.cause);
    passgen_error_free(&error);
    passgen_pattern_free(pattern);

    // a long chain of presets, each of which is nested deeply, is stopped
    // before it gets too deep, rather than resolved all the way down.
    char name[32], inner[32];
    assert(PASSGEN_OK == passgen_env_preset_add(env, "chain0", "a"));
    for(int i = 1; i < 1000; i++) {
        snprintf(name, sizeof(name), "chain%d", i);
        snprintf(inner, sizeof(inner), "\\p{chain%d}", i - 1);
        char *string = nested(500, inner);
        assert(string);
        assert(PASSGEN_OK == passgen_env_preset_add(env, name, string));
        free(string);
    }
    assert_eq(
        PASSGEN_ERROR_DEPTH_LIMIT,
        generate_seeded(env, "\\p{chain999}", 1, output));
    assert_eq(PASSGEN_OK, generate_seeded(env, "\\p{chain1}", 1, output));
    assert_streq(output, "a");

    passgen_env_free(env);
    return test_ok;
}

/// Presets which use the same ones many times are resolved once each, rather
/// than once for every time that they are used.
test_result test_preset_shared(void) {
    passgen_env *env = passgen_env_new(NULL);
    assert(env);

    // using double60 would generate 2^60 characters, but resolving it is
    // quick.
    char name[32], value[64];
    assert(PASSGEN_OK == passgen_env_preset_add(env, "double0", "a"));
    for(int i = 1; i <= 60; i++) {
        snprintf(name, sizeof(name), "double%d", i);
        snprintf(value, sizeof(value), "\\p{double%d}\\p{double%d}", i - 1, i - 1);
        assert(PASSGEN_OK == passgen_env_preset_add(env, name, value));
    }

    passgen_pattern *pattern;
    assert(PASSGEN_OK == passgen_parse(&pattern, NULL, "\\p{double60}"));
    assert_eq(PASSGEN_OK, passgen_resolve(pattern, env, NULL));
    passgen_pattern_free(pattern);

    char output[256];
    assert_eq(PASSGEN_OK, generate_seeded(env, "\\p{double3}", 1, output));
    assert_streq(output, "aaaaaaaa");

    passgen_env_free(env);
    return test_ok;
}

/// A pattern points to the presets it uses, so replacing one means that it
/// has to be resolved again.
test_result test_preset_replace(void) {
    passgen_random random;
    assert(passgen_random_open(&random, "zero"));
    passgen_env *env = passgen_env_new(&random);
    assert(env);
    assert(PASSGEN_OK == passgen_env_preset_add(env, "word", "old"));

    passgen_pattern *pattern;
    assert(PASSGEN_OK == passgen_parse(&pattern, NULL, "\\p{word}"));
    assert_eq(PASSGEN_OK, passgen_resolve(pattern, env, NULL));

    uint8_t output[16];
    size_t length = 0;
    assert_eq(
        PASSGEN_OK,
        passgen_generate_fill_utf8(pattern, env, NULL, NULL, output, 15, &length));
    output[length] = 0;
    assert_streq((char *) output, "old");

    assert(PASSGEN_OK == passgen_env_preset_add(env, "word", "new"));
    assert_eq(
        PASSGEN_ERROR_INVALID_ARGUMENT,
        passgen_generate_fill_utf8(pattern, env, NULL, NULL, output, 15, &length));

    assert_eq(PASSGEN_OK, passgen_resolve(pattern, env, NULL));
    assert_eq(
        PASSGEN_OK,
        passgen_generate_fill_utf8(pattern, env, NULL, NULL, output, 15, &length));
    output[length] = 0;
    assert_streq((char *) output, "new");

    passgen_pattern_free(pattern);
    passgen_env_free(env);
    passgen_random_close(&random);
    return test_ok;
}
