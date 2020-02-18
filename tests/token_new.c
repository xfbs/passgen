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
    assert(parser.data.unicode_payload.length == 0); \
    assert(parser.data.unicode_payload.codepoint == 0); \
    assert(token_parse(&parser, &token, a) == TOKEN_UNICODE_PAYLOAD); \
    assert(parser.state == TOKEN_UNICODE_PAYLOAD); \
    assert(parser.data.unicode_payload.length == 1); \
    assert(token_parse(&parser, &token, b) == TOKEN_UNICODE_PAYLOAD); \
    assert(parser.state == TOKEN_UNICODE_PAYLOAD); \
    assert(parser.data.unicode_payload.length == 2); \
    assert(token_parse(&parser, &token, '}') == TOKEN_INIT); \
    assert(parser.state == TOKEN_INIT); \
    assert(token.codepoint == out)

    TEST_UNICODE('f', 'a', 0xFA);
    TEST_UNICODE('a', '4', 0xA4);
    TEST_UNICODE('F', '1', 0xF1);
    TEST_UNICODE('F', 'c', 0xFC);
    TEST_UNICODE('d', '9', 0xD9);

#undef TEST_UNICODE

#define TEST_UNICODE(a, b, c, d, e, f, out) \
    assert(token_parse(&parser, &token, '\\') == TOKEN_ESCAPED); \
    assert(parser.state == TOKEN_ESCAPED); \
    assert(token_parse(&parser, &token, 'u') == TOKEN_UNICODE); \
    assert(parser.state == TOKEN_UNICODE); \
    assert(token_parse(&parser, &token, '{') == TOKEN_UNICODE_PAYLOAD); \
    assert(parser.state == TOKEN_UNICODE_PAYLOAD); \
    assert(parser.data.unicode_payload.length == 0); \
    assert(parser.data.unicode_payload.codepoint == 0); \
    assert(token_parse(&parser, &token, a) == TOKEN_UNICODE_PAYLOAD); \
    assert(parser.state == TOKEN_UNICODE_PAYLOAD); \
    assert(parser.data.unicode_payload.length == 1); \
    assert(token_parse(&parser, &token, b) == TOKEN_UNICODE_PAYLOAD); \
    assert(parser.state == TOKEN_UNICODE_PAYLOAD); \
    assert(parser.data.unicode_payload.length == 2); \
    assert(token_parse(&parser, &token, c) == TOKEN_UNICODE_PAYLOAD); \
    assert(parser.state == TOKEN_UNICODE_PAYLOAD); \
    assert(parser.data.unicode_payload.length == 3); \
    assert(token_parse(&parser, &token, d) == TOKEN_UNICODE_PAYLOAD); \
    assert(parser.state == TOKEN_UNICODE_PAYLOAD); \
    assert(parser.data.unicode_payload.length == 4); \
    assert(token_parse(&parser, &token, e) == TOKEN_UNICODE_PAYLOAD); \
    assert(parser.state == TOKEN_UNICODE_PAYLOAD); \
    assert(parser.data.unicode_payload.length == 5); \
    assert(token_parse(&parser, &token, f) == TOKEN_UNICODE_PAYLOAD); \
    assert(parser.state == TOKEN_UNICODE_PAYLOAD); \
    assert(parser.data.unicode_payload.length == 6); \
    assert(token_parse(&parser, &token, '}') == TOKEN_INIT); \
    assert(parser.state == TOKEN_INIT); \
    assert(token.codepoint == out)

    TEST_UNICODE('0', '1', 'F', '6', '4', '2', 0x1F642);
    TEST_UNICODE('0', '1', 'f', '6', '0', 'b', 0x1F60B);
    TEST_UNICODE('0', '0', '0', '0', 'f', 'c', 0xFC);

#undef TEST_UNICODE

    return test_ok;
}

test_result test_token_unicode_error_start(void) {
    struct token_parser parser = {0};
    struct token token = {0};

    // test that passing any character that is not an opening brace after \u
    // causes an error state (so \u{FC} is fine, but \u[ is not).

#define TEST_UNICODE_ERROR(a, b) \
    assert(token_parse(&parser, &token, '\\') == TOKEN_ESCAPED); \
    assert(parser.state == TOKEN_ESCAPED); \
    assert(token_parse(&parser, &token, 'u') == TOKEN_UNICODE); \
    assert(parser.state == TOKEN_UNICODE); \
    assert(token_parse(&parser, &token, a) == TOKEN_ERROR_UNICODE_START); \
    assert(parser.state == TOKEN_ERROR_UNICODE_START); \
    assert(token_parse(&parser, &token, b) == TOKEN_ERROR_UNICODE_START); \
    assert(parser.state == TOKEN_ERROR_UNICODE_START); \
    parser.state = TOKEN_INIT

    TEST_UNICODE_ERROR('x', 'y');
    TEST_UNICODE_ERROR('c', '{');
    TEST_UNICODE_ERROR('}', '{');
    TEST_UNICODE_ERROR('[', ']');

#undef TEST_UNICODE_ERROR

    return test_ok;
}

test_result test_token_unicode_error_payload(void) {
    struct token_parser parser = {0};
    struct token token = {0};

    // test that passing any character that is not an opening brace after \u
    // causes an error state (so \u{FC} is fine, but \u[ is not).

#define TEST_UNICODE_ERROR(a) \
    assert(token_parse(&parser, &token, '\\') == TOKEN_ESCAPED); \
    assert(parser.state == TOKEN_ESCAPED); \
    assert(token_parse(&parser, &token, 'u') == TOKEN_UNICODE); \
    assert(parser.state == TOKEN_UNICODE); \
    assert(token_parse(&parser, &token, '{') == TOKEN_UNICODE_PAYLOAD); \
    assert(parser.state == TOKEN_UNICODE_PAYLOAD); \
    assert(token_parse(&parser, &token, a) == TOKEN_ERROR_UNICODE_PAYLOAD); \
    assert(parser.state == TOKEN_ERROR_UNICODE_PAYLOAD); \
    parser.state = TOKEN_INIT

    TEST_UNICODE_ERROR('x');
    TEST_UNICODE_ERROR(' ');
    TEST_UNICODE_ERROR('-');
    TEST_UNICODE_ERROR('!');

#undef TEST_UNICODE_ERROR

    return test_ok;
}

test_result test_token_unicode_error_len(void) {
    struct token_parser parser = {0};
    struct token token = {0};

    // test that passing any character that is not an opening brace after \u
    // causes an error state (so \u{FC} is fine, but \u[ is not).

#define TEST_UNICODE_ERROR(a, b, c, d, e, f, g) \
    assert(token_parse(&parser, &token, '\\') == TOKEN_ESCAPED); \
    assert(parser.state == TOKEN_ESCAPED); \
    assert(token_parse(&parser, &token, 'u') == TOKEN_UNICODE); \
    assert(parser.state == TOKEN_UNICODE); \
    assert(token_parse(&parser, &token, '{') == TOKEN_UNICODE_PAYLOAD); \
    assert(parser.state == TOKEN_UNICODE_PAYLOAD); \
    assert(parser.data.unicode_payload.length == 0); \
    assert(parser.data.unicode_payload.codepoint == 0); \
    assert(token_parse(&parser, &token, a) == TOKEN_UNICODE_PAYLOAD); \
    assert(parser.state == TOKEN_UNICODE_PAYLOAD); \
    assert(parser.data.unicode_payload.length == 1); \
    assert(token_parse(&parser, &token, b) == TOKEN_UNICODE_PAYLOAD); \
    assert(parser.state == TOKEN_UNICODE_PAYLOAD); \
    assert(parser.data.unicode_payload.length == 2); \
    assert(token_parse(&parser, &token, c) == TOKEN_UNICODE_PAYLOAD); \
    assert(parser.state == TOKEN_UNICODE_PAYLOAD); \
    assert(parser.data.unicode_payload.length == 3); \
    assert(token_parse(&parser, &token, d) == TOKEN_UNICODE_PAYLOAD); \
    assert(parser.state == TOKEN_UNICODE_PAYLOAD); \
    assert(parser.data.unicode_payload.length == 4); \
    assert(token_parse(&parser, &token, e) == TOKEN_UNICODE_PAYLOAD); \
    assert(parser.state == TOKEN_UNICODE_PAYLOAD); \
    assert(parser.data.unicode_payload.length == 5); \
    assert(token_parse(&parser, &token, f) == TOKEN_UNICODE_PAYLOAD); \
    assert(parser.state == TOKEN_UNICODE_PAYLOAD); \
    assert(parser.data.unicode_payload.length == 6); \
    assert(token_parse(&parser, &token, f) == TOKEN_ERROR_UNICODE_PAYLOAD_LEN); \
    assert(parser.state == TOKEN_ERROR_UNICODE_PAYLOAD_LEN); \
    assert(parser.data.unicode_payload.length == 7); \
    parser.state = TOKEN_INIT

    TEST_UNICODE_ERROR('0', '0', '0', '0', '0', '0', '0');
    TEST_UNICODE_ERROR('1', '0', 'f', 'f', 'f', 'f', '0');

#undef TEST_UNICODE_ERROR

    return test_ok;
}
