#include "bench.h"
#include <assert.h>
#include <passgen/parser/token.h>
#include <passgen/pattern/token.h>
#include <stdlib.h>

#define CODEPOINT_CHAR(c)         c
#define CODEPOINT_ESCAPED_CHAR(c) '\\', c
#define CODEPOINT_UNICODE(...)    '\\', 'u', '{', __VA_ARGS__, '}'
#define BENCH_TOKEN_LINE_LEN      135

static uint32_t bench_token_chars[BENCH_TOKEN_LINE_LEN] = {
    // 34 simple chars (34 bytes)
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
    CODEPOINT_CHAR('o'),
    CODEPOINT_CHAR('p'),
    CODEPOINT_CHAR('q'),
    CODEPOINT_CHAR('r'),
    CODEPOINT_CHAR('s'),
    CODEPOINT_CHAR('t'),
    CODEPOINT_CHAR('u'),
    CODEPOINT_CHAR('v'),
    CODEPOINT_CHAR('w'),
    CODEPOINT_CHAR('x'),
    CODEPOINT_CHAR('y'),
    CODEPOINT_CHAR('z'),
    CODEPOINT_CHAR('('),
    CODEPOINT_CHAR(')'),
    CODEPOINT_CHAR('{'),
    CODEPOINT_CHAR('}'),
    CODEPOINT_CHAR('['),
    CODEPOINT_CHAR(']'),
    CODEPOINT_CHAR('-'),
    CODEPOINT_CHAR('_'),

    // 33 escaped chars (66 bytes)
    CODEPOINT_ESCAPED_CHAR('a'),
    CODEPOINT_ESCAPED_CHAR('b'),
    CODEPOINT_ESCAPED_CHAR('c'),
    CODEPOINT_ESCAPED_CHAR('d'),
    CODEPOINT_ESCAPED_CHAR('e'),
    CODEPOINT_ESCAPED_CHAR('f'),
    CODEPOINT_ESCAPED_CHAR('g'),
    CODEPOINT_ESCAPED_CHAR('h'),
    CODEPOINT_ESCAPED_CHAR('i'),
    CODEPOINT_ESCAPED_CHAR('j'),
    CODEPOINT_ESCAPED_CHAR('k'),
    CODEPOINT_ESCAPED_CHAR('l'),
    CODEPOINT_ESCAPED_CHAR('m'),
    CODEPOINT_ESCAPED_CHAR('n'),
    CODEPOINT_ESCAPED_CHAR('o'),
    CODEPOINT_ESCAPED_CHAR('p'),
    CODEPOINT_ESCAPED_CHAR('q'),
    CODEPOINT_ESCAPED_CHAR('r'),
    CODEPOINT_ESCAPED_CHAR('s'),
    CODEPOINT_ESCAPED_CHAR('t'),
    CODEPOINT_ESCAPED_CHAR('v'),
    CODEPOINT_ESCAPED_CHAR('w'),
    CODEPOINT_ESCAPED_CHAR('x'),
    CODEPOINT_ESCAPED_CHAR('y'),
    CODEPOINT_ESCAPED_CHAR('z'),
    CODEPOINT_ESCAPED_CHAR('('),
    CODEPOINT_ESCAPED_CHAR(')'),
    CODEPOINT_ESCAPED_CHAR('{'),
    CODEPOINT_ESCAPED_CHAR('}'),
    CODEPOINT_ESCAPED_CHAR('['),
    CODEPOINT_ESCAPED_CHAR(']'),
    CODEPOINT_ESCAPED_CHAR('-'),
    CODEPOINT_ESCAPED_CHAR('_'),

    // escaped unicode (35 bytes)
    CODEPOINT_UNICODE('a'),
    CODEPOINT_UNICODE('b'),
    CODEPOINT_UNICODE('a', 'b'),
    CODEPOINT_UNICODE('b', 'c'),
    CODEPOINT_UNICODE('e', 'f'),
    CODEPOINT_UNICODE('a', '1', 'b'),
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
    (void) opts;
    struct bench_token_data *data = malloc(sizeof(struct bench_token_data));
    data->repeat = 1000;
    return data;
}

static void *bench_token_iterate(void *raw_data) {
    struct bench_token_data *data = raw_data;
    passgen_token_parser_init(&data->parser);

    int ret;
    for(size_t r = 0; r < data->repeat; r++) {
        for(size_t i = 0; i < BENCH_TOKEN_LINE_LEN; i++) {
            ret = passgen_token_parse(
                &data->parser,
                &data->token,
                1,
                bench_token_chars[i]);
        }
    }

    assert(ret == PASSGEN_TOKEN_INIT);

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
