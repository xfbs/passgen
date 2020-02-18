#include "passgen/token_new.h"
#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include "tests.h"

test_result test_token_normal(void) {
    struct token_parser parser = {0};
    struct token token = {0};

    // test that normal tokens (that are not special escaped ones) get parsed
    // just the way they are.

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

#undef TEST_NORMAL

    return test_ok;
}

test_result test_token_escaped(void) {
    struct token_parser parser = {0};
    struct token token = {0};

    // test that simple escaped tokens get parsed correctly.

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

#undef TEST_ESCAPED

    return test_ok;
}

test_result test_token_unicode(void) {
    struct token_parser parser = {0};
    struct token token = {0};

    // test that simple escaped tokens get parsed correctly.

#define TEST_UNICODE(a, b, out) \
    assert(token_parse(&parser, &token, '\\') == TOKEN_ESCAPED); \
    assert(parser.state == TOKEN_ESCAPED); \
    assert(token_parse(&parser, &token, 'u') == TOKEN_UNICODE); \
    assert(parser.state == TOKEN_UNICODE); \
    assert(token_parse(&parser, &token, '{') == TOKEN_UNICODE_PAYLOAD); \
    assert(parser.state == TOKEN_UNICODE_PAYLOAD); \
    assert(token_parse(&parser, &token, a) == TOKEN_UNICODE_PAYLOAD); \
    assert(parser.state == TOKEN_UNICODE_PAYLOAD); \
    assert(token_parse(&parser, &token, b) == TOKEN_UNICODE_PAYLOAD); \
    assert(parser.state == TOKEN_UNICODE_PAYLOAD); \
    assert(token_parse(&parser, &token, '}') == TOKEN_INIT); \
    assert(parser.state == TOKEN_INIT); \
    assert(token.codepoint == out)

    TEST_UNICODE('f', 'a', 0xFA);
    TEST_UNICODE('a', '4', 0xA4);
    TEST_UNICODE('F', '1', 0xF1);
    TEST_UNICODE('F', 'c', 0xFC);
    TEST_UNICODE('d', '9', 0xD9);

#undef TEST_UNICODE

#define TEST_UNICODE(a, b, c, d, e, out) \
    assert(token_parse(&parser, &token, '\\') == TOKEN_ESCAPED); \
    assert(parser.state == TOKEN_ESCAPED); \
    assert(token_parse(&parser, &token, 'u') == TOKEN_UNICODE); \
    assert(parser.state == TOKEN_UNICODE); \
    assert(token_parse(&parser, &token, '{') == TOKEN_UNICODE_PAYLOAD); \
    assert(parser.state == TOKEN_UNICODE_PAYLOAD); \
    assert(token_parse(&parser, &token, a) == TOKEN_UNICODE_PAYLOAD); \
    assert(parser.state == TOKEN_UNICODE_PAYLOAD); \
    assert(token_parse(&parser, &token, b) == TOKEN_UNICODE_PAYLOAD); \
    assert(parser.state == TOKEN_UNICODE_PAYLOAD); \
    assert(token_parse(&parser, &token, c) == TOKEN_UNICODE_PAYLOAD); \
    assert(parser.state == TOKEN_UNICODE_PAYLOAD); \
    assert(token_parse(&parser, &token, d) == TOKEN_UNICODE_PAYLOAD); \
    assert(parser.state == TOKEN_UNICODE_PAYLOAD); \
    assert(token_parse(&parser, &token, e) == TOKEN_UNICODE_PAYLOAD); \
    assert(parser.state == TOKEN_UNICODE_PAYLOAD); \
    assert(token_parse(&parser, &token, '}') == TOKEN_INIT); \
    assert(parser.state == TOKEN_INIT); \
    assert(token.codepoint == out)

    TEST_UNICODE('1', 'F', '6', '4', '2', 0x1F642);
    TEST_UNICODE('1', 'f', '6', '0', 'b', 0x1F60B);
    TEST_UNICODE('0', '0', '0', 'f', 'c', 0xFC);

#undef TEST_UNICODE

    return test_ok;
}
