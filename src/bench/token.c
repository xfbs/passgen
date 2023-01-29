#include "bench.h"
#include <passgen/parser/token.h>
#include <passgen/pattern/token.h>
#include <stdlib.h>

#define CODEPOINT_CHAR(c) c
#define CODEPOINT_ESCAPED_CHAR(c) '\\', c
#define BENCH_TOKEN_LINE_LEN 32

static uint32_t bench_token_chars[BENCH_TOKEN_LINE_LEN] = {
    CODEPOINT_CHAR('a'),
    CODEPOINT_CHAR('b'),
    CODEPOINT_CHAR('c'),
    CODEPOINT_CHAR('d'),
    CODEPOINT_CHAR('e'),
    CODEPOINT_CHAR('f'),
    CODEPOINT_CHAR('g'),
    CODEPOINT_CHAR('h'),
    CODEPOINT_CHAR('i'),
    CODEPOINT_CHAR('j'),
    CODEPOINT_CHAR('k'),
    CODEPOINT_CHAR('l'),
    CODEPOINT_CHAR('m'),
    CODEPOINT_CHAR('n'),
};

struct bench_token_data {
    size_t repeat;
    passgen_token_parser parser;
    passgen_token token;
};

static double bench_token_mult(void *raw_data) {
    struct bench_token_data *data = raw_data;
    return BENCH_TOKEN_LINE_LEN * data->repeat;
}

static void *bench_token_prepare(const passgen_hashmap *opts) {
    struct bench_token_data *data = malloc(sizeof(struct bench_token_data));
    data->repeat = 100;
    return data;
}

static void *bench_token_iterate(void *raw_data) {
    struct bench_token_data *data = raw_data;
    passgen_token_parser_init(&data->parser);

    for(size_t r = 0; r < data->repeat; r++) {
        for(size_t i = 0; i < BENCH_TOKEN_LINE_LEN; i++) {
            passgen_token_parse(
                &data->parser,
                &data->token,
                1,
                bench_token_chars[i]);
        }
    }

    return NULL;
}

static void bench_token_release(void *data) {
    free(data);
}

const bench token_parse = {
    .name = "token_parse",
    .desc = "Token parse",
    .prepare = &bench_token_prepare,
    .iterate = &bench_token_iterate,
    .cleanup = NULL,
    .release = &bench_token_release,
    .consumes = false,
    .multiplier = &bench_token_mult,
    .unit = "chars",
};
