#include "passgen/token_new.h"
#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include "tests.h"

test_result test_token_normal(void) {
    struct token_parser parser = {0};
    struct token token = {0};

#define TEST_NORMAL(cp) \
    assert(token_parse(&parser, &token, cp) == TOKEN_INIT); \
    assert(parser.state == TOKEN_INIT); \
    assert(token.codepoint == cp); \
    assert(token.type == TOKEN_NORMAL); \
    assert(token.escaped == false)

    TEST_NORMAL('a');
    TEST_NORMAL('b');
    TEST_NORMAL('c');
    TEST_NORMAL(' ');
    TEST_NORMAL('!');
    TEST_NORMAL('[');
    TEST_NORMAL(']');

    return test_ok;
}

test_result test_token_escaped(void) {
    struct token_parser parser = {0};
    struct token token = {0};

#define TEST_ESCAPED(in, out) \
    assert(token_parse(&parser, &token, '\\') == TOKEN_ESCAPED); \
    assert(parser.state == TOKEN_ESCAPED); \
    assert(token_parse(&parser, &token, in) == TOKEN_INIT); \
    assert(parser.state == TOKEN_INIT); \
    assert(token.codepoint == out); \
    assert(token.type == TOKEN_NORMAL); \
    assert(token.escaped == true)

    TEST_ESCAPED('\\', '\\');
    TEST_ESCAPED('a', '\a');
    TEST_ESCAPED('b', '\b');
    TEST_ESCAPED('f', '\f');
    TEST_ESCAPED('r', '\r');
    TEST_ESCAPED('n', '\n');
    TEST_ESCAPED('e', '\033');

    return test_ok;
}
