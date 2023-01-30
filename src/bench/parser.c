#include "bench.h"
#include <assert.h>
#include <passgen/passgen.h>
#include <stdlib.h>
#include <string.h>

static const char *pattern_fragment = "abcde?fgh?i{9}j[klmopq]{2,19}(prst|uvw){15}\\u{68}\\u{65}\\u{6c}\\u{6C}\\u{6F}\\{thishello\\}this\\[hello\\][a\\-z][a-z0-9A-Z;':\",.<>]{10}\\w{wordlist}\\m{markov}\\p{pattern}";

struct bench_parser_data {
    size_t repeat;
    const char *input;
    passgen_pattern pattern;
};

static double bench_parser_mult(void *raw_data) {
    struct bench_parser_data *data = raw_data;
    return strlen(data->input);
}

static void *bench_parser_prepare(const passgen_hashmap *opts) {
    (void) opts;
    struct bench_parser_data *data = malloc(sizeof(struct bench_parser_data));
    data->repeat = 750;
    size_t input_len = data->repeat * strlen(pattern_fragment) + 1;
    char *input = malloc(input_len);
    size_t pos = 0;
    for(size_t i = 0; i < data->repeat; i++) {
        pos += sprintf(&input[pos], "%s", pattern_fragment);
    }
    input[input_len - 1] = 0;
    assert(strlen(input) <= input_len);
    data->input = input;
    return data;
}

static void *bench_parser_iterate(void *raw_data) {
    struct bench_parser_data *data = raw_data;
    passgen_error error;
    int ret = passgen_parse(&data->pattern, &error, data->input);
    assert(ret == 0);
    return data;
}

static void bench_parser_cleanup(void *raw_data) {
    struct bench_parser_data *data = raw_data;
    free((void *) data->input);
    passgen_pattern_free(&data->pattern);
    memset(data, 0, sizeof(*data));
    free(data);
}

const bench bench_passgen_parse = {
    .name = "passgen_parse",
    .desc = "Passgen parse utf8",
    .prepare = &bench_parser_prepare,
    .iterate = &bench_parser_iterate,
    .cleanup = &bench_parser_cleanup,
    .release = NULL,
    .consumes = true,
    .multiplier = &bench_parser_mult,
    .unit = "chars",
};
