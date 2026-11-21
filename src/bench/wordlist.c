/// @file wordlist.c
///
/// Benchmarks reading a wordlist, and building Markov chains from it. The
/// wordlist is the large EFF one, which scripts/fetch-wordlists.sh downloads,
/// unless another one is given with `-o wordlist=PATH`.
#include "bench.h"
#include <passgen/assert.h>
#include <passgen/markov.h>
#include <passgen/wordlist.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *bench_wordlist_path(const passgen_hashmap *opts) {
    passgen_hashmap_entry *entry = passgen_hashmap_lookup(opts, "wordlist");
    const char *path = entry ? entry->value : PASSGEN_BENCH_WORDLIST;

    FILE *file = fopen(path, "r");
    if(!file) {
        // this is said once, rather than for every benchmark which needs it.
        static bool said = false;
        if(!said) {
            fprintf(
                stderr,
                "note: skipping the benchmarks which need the wordlist %s, "
                "which cannot be opened. run scripts/fetch-wordlists.sh, or "
                "give one with -o wordlist=PATH.\n",
                path);
            said = true;
        }
        return NULL;
    }
    fclose(file);
    return path;
}

/// A wordlist to read, or which was read.
typedef struct {
    passgen_wordlist wordlist;
    size_t size;
    size_t length;
} bench_wordlist;

/// Opens the wordlist, without reading it.
static bench_wordlist *bench_wordlist_open(const passgen_hashmap *opts) {
    const char *path = bench_wordlist_path(opts);
    if(!path) {
        return NULL;
    }
    FILE *file = fopen(path, "r");
    passgen_assert(file);
    bench_wordlist *data = malloc(sizeof(bench_wordlist));
    passgen_wordlist_init(&data->wordlist, file);
    fseek(file, 0, SEEK_END);
    data->size = ftell(file);
    fseek(file, 0, SEEK_SET);
    return data;
}

static void bench_wordlist_free(void *raw_data) {
    bench_wordlist *data = raw_data;
    passgen_wordlist_free(&data->wordlist);
    free(data);
}

static double bench_wordlist_size(void *raw_data) {
    bench_wordlist *data = raw_data;
    return data->size;
}

static void *bench_wordlist_prepare_load(const passgen_hashmap *opts) {
    return bench_wordlist_open(opts);
}

// reads the file, splits it into words, and checks that they are UTF-8.
static void *bench_wordlist_load(void *raw_data) {
    bench_wordlist *data = raw_data;
    passgen_error error;
    passgen_status status = passgen_wordlist_parse(&data->wordlist, &error);
    passgen_assert(status == PASSGEN_OK);
    (void) status;
    return data;
}

const bench wordlist_load = {
    .name = "wordlist_load",
    .desc = "Reads a wordlist from its file, and splits it into words",
    .unit = "bytes",
    .consumes = true,
    .prepare = &bench_wordlist_prepare_load,
    .iterate = &bench_wordlist_load,
    .cleanup = &bench_wordlist_free,
    .multiplier = &bench_wordlist_size,
};

/// Reads the wordlist, for building a Markov chain of a length from it.
static void *
bench_wordlist_prepare_markov(const passgen_hashmap *opts, size_t length) {
    bench_wordlist *data = bench_wordlist_open(opts);
    if(data) {
        bench_wordlist_load(data);
        data->length = length;
    }
    return data;
}

static double bench_wordlist_count(void *raw_data) {
    bench_wordlist *data = raw_data;
    return data->wordlist.count;
}

static void *bench_markov_build(void *raw_data) {
    bench_wordlist *data = raw_data;
    passgen_error error;
    passgen_status status =
        passgen_wordlist_parse_markov(&data->wordlist, data->length, &error);
    passgen_assert(status == PASSGEN_OK);
    (void) status;
    return data;
}

#define BENCH_MARKOV_BUILD(length)                                            \
    static void *bench_markov_prepare_##length(const passgen_hashmap *opts) { \
        return bench_wordlist_prepare_markov(opts, length);                   \
    }                                                                         \
    const bench markov_build_##length = {                                     \
        .name = "markov_build_" #length,                                      \
        .desc = "Builds a Markov chain of length " #length " from words",     \
        .unit = "words",                                                      \
        .consumes = true,                                                     \
        .prepare = &bench_markov_prepare_##length,                            \
        .iterate = &bench_markov_build,                                       \
        .cleanup = &bench_wordlist_free,                                      \
        .multiplier = &bench_wordlist_count,                                  \
    };

BENCH_MARKOV_BUILD(0)
BENCH_MARKOV_BUILD(1)
BENCH_MARKOV_BUILD(2)
BENCH_MARKOV_BUILD(3)
BENCH_MARKOV_BUILD(4)
BENCH_MARKOV_BUILD(5)
BENCH_MARKOV_BUILD(6)
BENCH_MARKOV_BUILD(7)
BENCH_MARKOV_BUILD(8)
