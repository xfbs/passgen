#include "passgen/token.h"
#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include "tests.h"

test_result test_token_normal(void) {
    struct passgen_token_parser parser = {0};
    struct passgen_token token = {0};

    // test that normal tokens (that are not special escaped ones) get parsed
    // just the way they are.

#define TEST_NORMAL(cp) \
    assert(passgen_token_parse(&parser, &token, cp) == PASSGEN_TOKEN_INIT); \
    assert(parser.state == PASSGEN_TOKEN_INIT); \
    assert(token.codepoint == cp); \
    assert(token.type == PASSGEN_TOKEN_NORMAL); \
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
    struct passgen_token_parser parser = {0};
    struct passgen_token token = {0};

    // test that simple escaped tokens get parsed correctly.

#define TEST_ESCAPED(in, out) \
    assert(passgen_token_parse(&parser, &token, '\\') == PASSGEN_TOKEN_ESCAPED); \
    assert(parser.state == PASSGEN_TOKEN_ESCAPED); \
    assert(passgen_token_parse(&parser, &token, in) == PASSGEN_TOKEN_INIT); \
    assert(parser.state == PASSGEN_TOKEN_INIT); \
    assert(token.codepoint == out); \
    assert(token.type == PASSGEN_TOKEN_NORMAL); \
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
    struct passgen_token_parser parser = {0};
    struct passgen_token token = {0};

    // test that simple escaped tokens get parsed correctly.

#define TEST_UNICODE(a, b, out) \
    assert(passgen_token_parse(&parser, &token, '\\') == PASSGEN_TOKEN_ESCAPED); \
    assert(parser.state == PASSGEN_TOKEN_ESCAPED); \
    assert(passgen_token_parse(&parser, &token, 'u') == PASSGEN_TOKEN_UNICODE); \
    assert(parser.state == PASSGEN_TOKEN_UNICODE); \
    assert(passgen_token_parse(&parser, &token, '{') == PASSGEN_TOKEN_UNICODE_PAYLOAD); \
    assert(parser.state == PASSGEN_TOKEN_UNICODE_PAYLOAD); \
    assert(parser.data.unicode_payload.length == 0); \
    assert(parser.data.unicode_payload.codepoint == 0); \
    assert(passgen_token_parse(&parser, &token, a) == PASSGEN_TOKEN_UNICODE_PAYLOAD); \
    assert(parser.state == PASSGEN_TOKEN_UNICODE_PAYLOAD); \
    assert(parser.data.unicode_payload.length == 1); \
    assert(passgen_token_parse(&parser, &token, b) == PASSGEN_TOKEN_UNICODE_PAYLOAD); \
    assert(parser.state == PASSGEN_TOKEN_UNICODE_PAYLOAD); \
    assert(parser.data.unicode_payload.length == 2); \
    assert(passgen_token_parse(&parser, &token, '}') == PASSGEN_TOKEN_INIT); \
    assert(parser.state == PASSGEN_TOKEN_INIT); \
    assert(token.codepoint == out)

    TEST_UNICODE('f', 'a', 0xFA);
    TEST_UNICODE('a', '4', 0xA4);
    TEST_UNICODE('F', '1', 0xF1);
    TEST_UNICODE('F', 'c', 0xFC);
    TEST_UNICODE('d', '9', 0xD9);

#undef TEST_UNICODE

#define TEST_UNICODE(a, b, c, d, e, f, out) \
    assert(passgen_token_parse(&parser, &token, '\\') == PASSGEN_TOKEN_ESCAPED); \
    assert(parser.state == PASSGEN_TOKEN_ESCAPED); \
    assert(passgen_token_parse(&parser, &token, 'u') == PASSGEN_TOKEN_UNICODE); \
    assert(parser.state == PASSGEN_TOKEN_UNICODE); \
    assert(passgen_token_parse(&parser, &token, '{') == PASSGEN_TOKEN_UNICODE_PAYLOAD); \
    assert(parser.state == PASSGEN_TOKEN_UNICODE_PAYLOAD); \
    assert(parser.data.unicode_payload.length == 0); \
    assert(parser.data.unicode_payload.codepoint == 0); \
    assert(passgen_token_parse(&parser, &token, a) == PASSGEN_TOKEN_UNICODE_PAYLOAD); \
    assert(parser.state == PASSGEN_TOKEN_UNICODE_PAYLOAD); \
    assert(parser.data.unicode_payload.length == 1); \
    assert(passgen_token_parse(&parser, &token, b) == PASSGEN_TOKEN_UNICODE_PAYLOAD); \
    assert(parser.state == PASSGEN_TOKEN_UNICODE_PAYLOAD); \
    assert(parser.data.unicode_payload.length == 2); \
    assert(passgen_token_parse(&parser, &token, c) == PASSGEN_TOKEN_UNICODE_PAYLOAD); \
    assert(parser.state == PASSGEN_TOKEN_UNICODE_PAYLOAD); \
    assert(parser.data.unicode_payload.length == 3); \
    assert(passgen_token_parse(&parser, &token, d) == PASSGEN_TOKEN_UNICODE_PAYLOAD); \
    assert(parser.state == PASSGEN_TOKEN_UNICODE_PAYLOAD); \
    assert(parser.data.unicode_payload.length == 4); \
    assert(passgen_token_parse(&parser, &token, e) == PASSGEN_TOKEN_UNICODE_PAYLOAD); \
    assert(parser.state == PASSGEN_TOKEN_UNICODE_PAYLOAD); \
    assert(parser.data.unicode_payload.length == 5); \
    assert(passgen_token_parse(&parser, &token, f) == PASSGEN_TOKEN_UNICODE_PAYLOAD); \
    assert(parser.state == PASSGEN_TOKEN_UNICODE_PAYLOAD); \
    assert(parser.data.unicode_payload.length == 6); \
    assert(passgen_token_parse(&parser, &token, '}') == PASSGEN_TOKEN_INIT); \
    assert(parser.state == PASSGEN_TOKEN_INIT); \
    assert(token.codepoint == out)

    TEST_UNICODE('0', '1', 'F', '6', '4', '2', 0x1F642);
    TEST_UNICODE('0', '1', 'f', '6', '0', 'b', 0x1F60B);
    TEST_UNICODE('0', '0', '0', '0', 'f', 'c', 0xFC);

#undef TEST_UNICODE

    return test_ok;
}

test_result test_token_unicode_error_start(void) {
    struct passgen_token_parser parser = {0};
    struct passgen_token token = {0};

    // test that passing any character that is not an opening brace after \u
    // causes an error state (so \u{FC} is fine, but \u[ is not).

#define TEST_UNICODE_ERROR(a, b) \
    assert(passgen_token_parse(&parser, &token, '\\') == PASSGEN_TOKEN_ESCAPED); \
    assert(parser.state == PASSGEN_TOKEN_ESCAPED); \
    assert(passgen_token_parse(&parser, &token, 'u') == PASSGEN_TOKEN_UNICODE); \
    assert(parser.state == PASSGEN_TOKEN_UNICODE); \
    assert(passgen_token_parse(&parser, &token, a) == PASSGEN_TOKEN_ERROR_UNICODE_START); \
    assert(parser.state == PASSGEN_TOKEN_ERROR_UNICODE_START); \
    assert(passgen_token_parse(&parser, &token, b) == PASSGEN_TOKEN_ERROR_UNICODE_START); \
    assert(parser.state == PASSGEN_TOKEN_ERROR_UNICODE_START); \
    parser.state = PASSGEN_TOKEN_INIT

    TEST_UNICODE_ERROR('x', 'y');
    TEST_UNICODE_ERROR('c', '{');
    TEST_UNICODE_ERROR('}', '{');
    TEST_UNICODE_ERROR('[', ']');

#undef TEST_UNICODE_ERROR

    return test_ok;
}

test_result test_token_unicode_error_payload(void) {
    struct passgen_token_parser parser = {0};
    struct passgen_token token = {0};

    // test that passing any character that is not an opening brace after \u
    // causes an error state (so \u{FC} is fine, but \u[ is not).

#define TEST_UNICODE_ERROR(a) \
    assert(passgen_token_parse(&parser, &token, '\\') == PASSGEN_TOKEN_ESCAPED); \
    assert(parser.state == PASSGEN_TOKEN_ESCAPED); \
    assert(passgen_token_parse(&parser, &token, 'u') == PASSGEN_TOKEN_UNICODE); \
    assert(parser.state == PASSGEN_TOKEN_UNICODE); \
    assert(passgen_token_parse(&parser, &token, '{') == PASSGEN_TOKEN_UNICODE_PAYLOAD); \
    assert(parser.state == PASSGEN_TOKEN_UNICODE_PAYLOAD); \
    assert(passgen_token_parse(&parser, &token, a) == PASSGEN_TOKEN_ERROR_UNICODE_PAYLOAD); \
    assert(parser.state == PASSGEN_TOKEN_ERROR_UNICODE_PAYLOAD); \
    parser.state = PASSGEN_TOKEN_INIT

    TEST_UNICODE_ERROR('x');
    TEST_UNICODE_ERROR(' ');
    TEST_UNICODE_ERROR('-');
    TEST_UNICODE_ERROR('!');

#undef TEST_UNICODE_ERROR

    return test_ok;
}

test_result test_token_unicode_error_len(void) {
    struct passgen_token_parser parser = {0};
    struct passgen_token token = {0};

    // test that passing any character that is not an opening brace after \u
    // causes an error state (so \u{FC} is fine, but \u[ is not).

#define TEST_UNICODE_ERROR(a, b, c, d, e, f, g) \
    assert(passgen_token_parse(&parser, &token, '\\') == PASSGEN_TOKEN_ESCAPED); \
    assert(parser.state == PASSGEN_TOKEN_ESCAPED); \
    assert(passgen_token_parse(&parser, &token, 'u') == PASSGEN_TOKEN_UNICODE); \
    assert(parser.state == PASSGEN_TOKEN_UNICODE); \
    assert(passgen_token_parse(&parser, &token, '{') == PASSGEN_TOKEN_UNICODE_PAYLOAD); \
    assert(parser.state == PASSGEN_TOKEN_UNICODE_PAYLOAD); \
    assert(parser.data.unicode_payload.length == 0); \
    assert(parser.data.unicode_payload.codepoint == 0); \
    assert(passgen_token_parse(&parser, &token, a) == PASSGEN_TOKEN_UNICODE_PAYLOAD); \
    assert(parser.state == PASSGEN_TOKEN_UNICODE_PAYLOAD); \
    assert(parser.data.unicode_payload.length == 1); \
    assert(passgen_token_parse(&parser, &token, b) == PASSGEN_TOKEN_UNICODE_PAYLOAD); \
    assert(parser.state == PASSGEN_TOKEN_UNICODE_PAYLOAD); \
    assert(parser.data.unicode_payload.length == 2); \
    assert(passgen_token_parse(&parser, &token, c) == PASSGEN_TOKEN_UNICODE_PAYLOAD); \
    assert(parser.state == PASSGEN_TOKEN_UNICODE_PAYLOAD); \
    assert(parser.data.unicode_payload.length == 3); \
    assert(passgen_token_parse(&parser, &token, d) == PASSGEN_TOKEN_UNICODE_PAYLOAD); \
    assert(parser.state == PASSGEN_TOKEN_UNICODE_PAYLOAD); \
    assert(parser.data.unicode_payload.length == 4); \
    assert(passgen_token_parse(&parser, &token, e) == PASSGEN_TOKEN_UNICODE_PAYLOAD); \
    assert(parser.state == PASSGEN_TOKEN_UNICODE_PAYLOAD); \
    assert(parser.data.unicode_payload.length == 5); \
    assert(passgen_token_parse(&parser, &token, f) == PASSGEN_TOKEN_UNICODE_PAYLOAD); \
    assert(parser.state == PASSGEN_TOKEN_UNICODE_PAYLOAD); \
    assert(parser.data.unicode_payload.length == 6); \
    assert(passgen_token_parse(&parser, &token, f) == PASSGEN_TOKEN_ERROR_UNICODE_PAYLOAD_LEN); \
    assert(parser.state == PASSGEN_TOKEN_ERROR_UNICODE_PAYLOAD_LEN); \
    assert(parser.data.unicode_payload.length == 7); \
    parser.state = PASSGEN_TOKEN_INIT

    TEST_UNICODE_ERROR('0', '0', '0', '0', '0', '0', '0');
    TEST_UNICODE_ERROR('1', '0', 'f', 'f', 'f', 'f', '0');

#undef TEST_UNICODE_ERROR

    return test_ok;
}
