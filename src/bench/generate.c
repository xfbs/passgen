/// @file generate.c
///
/// Benchmarks generating from patterns, as the command-line tool does: with
/// and without the entropy, and with every sequence from its own stream (as
/// with `--amount`). The randomness is ChaCha20 (which master passphrases
/// use), or xorshift in builds without it, so that it is the same in every
/// run.
#include "bench.h"
#include <passgen/assert.h>
#include <passgen/passgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/// How many sequences one iteration generates.
#define BENCH_GENERATE_COUNT 100

const char *bench_wordlist_path(const passgen_hashmap *opts);

/// A pattern to generate from, and what it needs.
typedef struct {
    passgen_random random;
    passgen_env *env;
    passgen_pattern *pattern;
    bool entropy;
    bool streams;
} bench_generate;

static void bench_random_open(passgen_random *random) {
#ifdef PASSGEN_MONOCYPHER
    passgen_random_chacha20_open(
        random,
        (const uint8_t *) "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
        (const uint8_t *) "passgen");
#else
    passgen_random_xorshift_open(random, 23456945749UL);
#endif
}

/// Adds the wordlist as `words`, if the pattern needs one. Returns false if
/// it is not there.
static bool bench_env_wordlist(
    passgen_env *env,
    const passgen_hashmap *opts,
    const char *pattern) {
    if(!strstr(pattern, "{words")) {
        return true;
    }
    const char *path = bench_wordlist_path(opts);
    if(!path) {
        return false;
    }
    passgen_status status =
        passgen_env_wordlist_add_path(env, "words", path, NULL);
    passgen_assert(status == PASSGEN_OK);
    (void) status;
    return true;
}

static void *bench_generate_prepare(
    const passgen_hashmap *opts,
    const char *pattern,
    bool entropy,
    bool streams) {
    bench_generate *data = calloc(1, sizeof(bench_generate));
    bench_random_open(&data->random);
    data->env = passgen_env_new(&data->random);
    data->entropy = entropy;
    data->streams = streams;

    if(!bench_env_wordlist(data->env, opts, pattern)) {
        passgen_env_free(data->env);
        passgen_random_close(&data->random);
        free(data);
        return NULL;
    }

    // this reads the wordlist and builds the Markov chains, which is not
    // part of what is measured here.
    passgen_error error;
    passgen_status status = passgen_parse(&data->pattern, &error, pattern);
    passgen_assert(status == PASSGEN_OK);
    status = passgen_resolve(data->pattern, data->env, &error);
    passgen_assert(status == PASSGEN_OK);
    (void) status;
    return data;
}

static void bench_generate_release(void *raw_data) {
    bench_generate *data = raw_data;
    passgen_pattern_free(data->pattern);
    passgen_env_free(data->env);
    passgen_random_close(&data->random);
    free(data);
}

static double bench_generate_count(void *raw_data) {
    (void) raw_data;
    return BENCH_GENERATE_COUNT;
}

static void *bench_generate_iterate(void *raw_data) {
    bench_generate *data = raw_data;
    uint8_t buffer[1024];
    for(size_t i = 0; i < BENCH_GENERATE_COUNT; i++) {
        if(data->streams) {
            passgen_random_stream(&data->random, i);
        }
        double entropy;
        size_t written;
        passgen_error error;
        passgen_status status = passgen_generate_fill_utf8(
            data->pattern,
            data->env,
            &error,
            data->entropy ? &entropy : NULL,
            buffer,
            sizeof(buffer),
            &written);
        passgen_assert(status == PASSGEN_OK);
        (void) status;
    }
    return NULL;
}

#define BENCH_PATTERN_APPLE2 "([a-zA-Z0-9]{6}-){2}[a-zA-Z0-9]{6}"
#define BENCH_PATTERN_WORDS  "\\w{words}(-\\w{words}){5}"
#define BENCH_PATTERN_MARKOV "\\m{words}(-\\m{words}){5}"

#define BENCH_GENERATE(id, pattern, entropy, streams, what)                 \
    static void *bench_generate_prepare_##id(const passgen_hashmap *opts) { \
        return bench_generate_prepare(opts, pattern, entropy, streams);     \
    }                                                                       \
    const bench generate_##id = {                                           \
        .name = "generate_" #id,                                            \
        .desc = what,                                                       \
        .unit = "outputs",                                                  \
        .prepare = &bench_generate_prepare_##id,                            \
        .iterate = &bench_generate_iterate,                                 \
        .release = &bench_generate_release,                                 \
        .multiplier = &bench_generate_count,                                \
    };

BENCH_GENERATE(
    apple2,
    BENCH_PATTERN_APPLE2,
    false,
    false,
    "Generates from sets, like the apple2 preset")
BENCH_GENERATE(
    apple2_entropy,
    BENCH_PATTERN_APPLE2,
    true,
    false,
    "Generates from sets, like the apple2 preset, with the entropy")
BENCH_GENERATE(
    apple2_streams,
    BENCH_PATTERN_APPLE2,
    false,
    true,
    "Generates from sets, each output from its own stream")
BENCH_GENERATE(
    words,
    BENCH_PATTERN_WORDS,
    false,
    false,
    "Generates six words from the wordlist")
BENCH_GENERATE(
    words_entropy,
    BENCH_PATTERN_WORDS,
    true,
    false,
    "Generates six words from the wordlist, with the entropy")
BENCH_GENERATE(
    markov,
    BENCH_PATTERN_MARKOV,
    false,
    false,
    "Generates six words from a Markov chain of length 3")
BENCH_GENERATE(
    markov_entropy,
    BENCH_PATTERN_MARKOV,
    true,
    false,
    "Generates six words from a Markov chain of length 3, with the entropy")

/// The presets which are resolved, which use each other.
static const char *bench_presets[][2] = {
    {"_char", "[a-zA-Z0-9]"},
    {"_block", "\\p{_char}{6}"},
    {"apple2", "(\\p{_block}-){2}\\p{_block}"},
    {"apple3", "\\p{apple2}-\\p{apple2}"},
};

static void *bench_resolve_prepare(const passgen_hashmap *opts) {
    (void) opts;
    passgen_env *env = passgen_env_new(NULL);
    for(size_t i = 0; i < sizeof(bench_presets) / sizeof(bench_presets[0]);
        i++) {
        passgen_status status = passgen_env_preset_add(
            env,
            bench_presets[i][0],
            bench_presets[i][1]);
        passgen_assert(status == PASSGEN_OK);
        (void) status;
    }
    return env;
}

// presets are parsed when they are first resolved, and kept in the
// environment, so every iteration has a new one.
static void *bench_resolve_iterate(void *raw_data) {
    passgen_env *env = raw_data;
    passgen_pattern *pattern;
    passgen_error error;
    passgen_status status = passgen_parse(&pattern, &error, "\\p{apple3}");
    passgen_assert(status == PASSGEN_OK);
    status = passgen_resolve(pattern, env, &error);
    passgen_assert(status == PASSGEN_OK);
    (void) status;
    passgen_pattern_free(pattern);
    return env;
}

static void bench_resolve_cleanup(void *raw_data) {
    passgen_env_free(raw_data);
}

const bench resolve_presets = {
    .name = "resolve_presets",
    .desc = "Parses a pattern and the presets it uses",
    .unit = "patterns",
    .consumes = true,
    .prepare = &bench_resolve_prepare,
    .iterate = &bench_resolve_iterate,
    .cleanup = &bench_resolve_cleanup,
};
