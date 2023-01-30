#include "passgen/pattern/token.h"
#include "passgen/parser/token.h"
#include "tests.h"

#define CODEPOINT_MAX 0x10FFFF

// Test that normal (non-escaped) tokens are parsed corredly. They should
// just be fed through.
test_result test_token_normal(void) {
    struct passgen_token_parser parser;
    struct passgen_token token;
    passgen_token_parser_init(&parser);

    for(uint32_t codepoint = 0; codepoint < CODEPOINT_MAX; codepoint++) {
        if(codepoint == '\\') {
            continue;
        }
        int ret = passgen_token_parse(&parser, &token, 1, codepoint);
        assert_eq(ret, PASSGEN_TOKEN_INIT);
        assert_eq(parser.state, PASSGEN_TOKEN_INIT);
        assert_eq(token.codepoint, codepoint);
    }

    return test_ok;
}

// Test that regular escapes are handled correctly: they should resolve
// to the appropriate characters.
test_result test_token_escaped(void) {
    struct passgen_token_parser parser;
    struct passgen_token token;
    passgen_token_parser_init(&parser);

    uint32_t escapes[][2] = {
        {'\\', '\\'},
        {'a', '\a'},
        {'b', '\b'},
        {'f', '\f'},
        {'r', '\r'},
        {'n', '\n'},
        {'t', '\t'},
        {'v', '\v'},
        {'e', '\033'},
        {0, 0}
    };

    for(size_t i = 0; escapes[i][0]; i++) {
        assert_eq(
            passgen_token_parse(&parser, &token, 1, '\\'),
            PASSGEN_TOKEN_ESCAPED);
        assert_eq(parser.state, PASSGEN_TOKEN_ESCAPED);
        assert_eq(
            passgen_token_parse(&parser, &token, 1, escapes[i][0]),
            PASSGEN_TOKEN_INIT);
        assert_eq(parser.state, PASSGEN_TOKEN_INIT);
        assert_eq(token.codepoint, escapes[i][1]);
    }

    return test_ok;
}

// Test that regular characters have the escape bit set when parsed with
// leading backslashes.
test_result test_token_special_escaped(void) {
    struct passgen_token_parser parser;
    struct passgen_token token;
    passgen_token_parser_init(&parser);

    for(uint32_t codepoint = 0; codepoint < CODEPOINT_MAX; codepoint++) {
        switch(codepoint) {
            case '\\':
            case 'a':
            case 'b':
            case 'f':
            case 'r':
            case 'n':
            case 't':
            case 'v':
            case 'u':
            case 'e':
                continue;
            default:
                break;
        }
        int ret = passgen_token_parse(&parser, &token, 1, '\\');
        assert_eq(ret, PASSGEN_TOKEN_ESCAPED);
        assert_eq(parser.state, PASSGEN_TOKEN_ESCAPED);

        ret = passgen_token_parse(&parser, &token, 1, codepoint);
        assert_eq(ret, PASSGEN_TOKEN_INIT);
        assert_eq(parser.state, PASSGEN_TOKEN_INIT);
        assert_eq(token.codepoint, (codepoint | PASSGEN_TOKEN_ESCAPED_BIT));
    }

    return test_ok;
}

// Test that escaped unicode tokens (eg: \u{ffff}) get parsed correctly.
test_result test_token_unicode(void) {
    struct passgen_token_parser parser = {0};
    struct passgen_token token = {0};

    char buffer[8];
    for(uint32_t codepoint = 0; codepoint < CODEPOINT_MAX; codepoint++) {
        sprintf(buffer, "%x", codepoint);
        int ret = passgen_token_parse(&parser, &token, 1, '\\');
        assert_eq(ret, PASSGEN_TOKEN_ESCAPED);
        ret = passgen_token_parse(&parser, &token, 1, 'u');
        assert_eq(ret, PASSGEN_TOKEN_UNICODE);
        ret = passgen_token_parse(&parser, &token, 1, '{');
        assert_eq(ret, PASSGEN_TOKEN_UNICODE_PAYLOAD);
        assert_eq(parser.data.unicode_payload.length, 0);
        for(size_t i = 0; buffer[i]; i++) {
            ret = passgen_token_parse(&parser, &token, 1, buffer[i]);
            assert_eq(ret, PASSGEN_TOKEN_UNICODE_PAYLOAD);
            assert_eq(parser.data.unicode_payload.length, i + 1);
        }
        ret = passgen_token_parse(&parser, &token, 1, '}');
        assert_eq(ret, PASSGEN_TOKEN_INIT);
        assert_eq(token.codepoint, codepoint);
    }

    return test_ok;
}

test_result test_token_unicode_error_start(void) {
    struct passgen_token_parser parser = {0};
    struct passgen_token token = {0};

    // test that passing any character that is not an opening brace after \u
    // causes an error state (so \u{FC} is fine, but \u[ is not).

#define TEST_UNICODE_ERROR(a, b)                               \
    assert(                                                    \
        passgen_token_parse(&parser, &token, 1, '\\') ==       \
        PASSGEN_TOKEN_ESCAPED);                                \
    assert(parser.state == PASSGEN_TOKEN_ESCAPED);             \
    assert(                                                    \
        passgen_token_parse(&parser, &token, 1, 'u') ==        \
        PASSGEN_TOKEN_UNICODE);                                \
    assert(parser.state == PASSGEN_TOKEN_UNICODE);             \
    assert(                                                    \
        passgen_token_parse(&parser, &token, 1, a) ==          \
        PASSGEN_TOKEN_ERROR_UNICODE_START);                    \
    assert(parser.state == PASSGEN_TOKEN_ERROR_UNICODE_START); \
    assert(                                                    \
        passgen_token_parse(&parser, &token, 1, b) ==          \
        PASSGEN_TOKEN_ERROR_UNICODE_START);                    \
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

#define TEST_UNICODE_ERROR(a)                                    \
    assert(                                                      \
        passgen_token_parse(&parser, &token, 1, '\\') ==         \
        PASSGEN_TOKEN_ESCAPED);                                  \
    assert(parser.state == PASSGEN_TOKEN_ESCAPED);               \
    assert(                                                      \
        passgen_token_parse(&parser, &token, 1, 'u') ==          \
        PASSGEN_TOKEN_UNICODE);                                  \
    assert(parser.state == PASSGEN_TOKEN_UNICODE);               \
    assert(                                                      \
        passgen_token_parse(&parser, &token, 1, '{') ==          \
        PASSGEN_TOKEN_UNICODE_PAYLOAD);                          \
    assert(parser.state == PASSGEN_TOKEN_UNICODE_PAYLOAD);       \
    assert(                                                      \
        passgen_token_parse(&parser, &token, 1, a) ==            \
        PASSGEN_TOKEN_ERROR_UNICODE_PAYLOAD);                    \
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

#define TEST_UNICODE_ERROR(a, b, c, d, e, f, g)                      \
    assert(                                                          \
        passgen_token_parse(&parser, &token, 1, '\\') ==             \
        PASSGEN_TOKEN_ESCAPED);                                      \
    assert(parser.state == PASSGEN_TOKEN_ESCAPED);                   \
    assert(                                                          \
        passgen_token_parse(&parser, &token, 1, 'u') ==              \
        PASSGEN_TOKEN_UNICODE);                                      \
    assert(parser.state == PASSGEN_TOKEN_UNICODE);                   \
    assert(                                                          \
        passgen_token_parse(&parser, &token, 1, '{') ==              \
        PASSGEN_TOKEN_UNICODE_PAYLOAD);                              \
    assert(parser.state == PASSGEN_TOKEN_UNICODE_PAYLOAD);           \
    assert(parser.data.unicode_payload.length == 0);                 \
    assert(parser.data.unicode_payload.codepoint == 0);              \
    assert(                                                          \
        passgen_token_parse(&parser, &token, 1, a) ==                \
        PASSGEN_TOKEN_UNICODE_PAYLOAD);                              \
    assert(parser.state == PASSGEN_TOKEN_UNICODE_PAYLOAD);           \
    assert(parser.data.unicode_payload.length == 1);                 \
    assert(                                                          \
        passgen_token_parse(&parser, &token, 1, b) ==                \
        PASSGEN_TOKEN_UNICODE_PAYLOAD);                              \
    assert(parser.state == PASSGEN_TOKEN_UNICODE_PAYLOAD);           \
    assert(parser.data.unicode_payload.length == 2);                 \
    assert(                                                          \
        passgen_token_parse(&parser, &token, 1, c) ==                \
        PASSGEN_TOKEN_UNICODE_PAYLOAD);                              \
    assert(parser.state == PASSGEN_TOKEN_UNICODE_PAYLOAD);           \
    assert(parser.data.unicode_payload.length == 3);                 \
    assert(                                                          \
        passgen_token_parse(&parser, &token, 1, d) ==                \
        PASSGEN_TOKEN_UNICODE_PAYLOAD);                              \
    assert(parser.state == PASSGEN_TOKEN_UNICODE_PAYLOAD);           \
    assert(parser.data.unicode_payload.length == 4);                 \
    assert(                                                          \
        passgen_token_parse(&parser, &token, 1, e) ==                \
        PASSGEN_TOKEN_UNICODE_PAYLOAD);                              \
    assert(parser.state == PASSGEN_TOKEN_UNICODE_PAYLOAD);           \
    assert(parser.data.unicode_payload.length == 5);                 \
    assert(                                                          \
        passgen_token_parse(&parser, &token, 1, f) ==                \
        PASSGEN_TOKEN_UNICODE_PAYLOAD);                              \
    assert(parser.state == PASSGEN_TOKEN_UNICODE_PAYLOAD);           \
    assert(parser.data.unicode_payload.length == 6);                 \
    assert(                                                          \
        passgen_token_parse(&parser, &token, 1, f) ==                \
        PASSGEN_TOKEN_ERROR_UNICODE_PAYLOAD_LEN);                    \
    assert(parser.state == PASSGEN_TOKEN_ERROR_UNICODE_PAYLOAD_LEN); \
    assert(parser.data.unicode_payload.length == 7);                 \
    parser.state = PASSGEN_TOKEN_INIT

    TEST_UNICODE_ERROR('0', '0', '0', '0', '0', '0', '0');
    TEST_UNICODE_ERROR('1', '0', 'f', 'f', 'f', 'f', '0');

#undef TEST_UNICODE_ERROR

    return test_ok;
}

test_result test_token_state_string(void) {
    // initial state
    assert(passgen_token_state_string(PASSGEN_TOKEN_INIT) != NULL);

    // multi-charpoint states
    assert(passgen_token_state_string(PASSGEN_TOKEN_ESCAPED) != NULL);
    assert(passgen_token_state_string(PASSGEN_TOKEN_UNICODE) != NULL);
    assert(passgen_token_state_string(PASSGEN_TOKEN_UNICODE_PAYLOAD) != NULL);

    // error states
    assert(
        passgen_token_state_string(PASSGEN_TOKEN_ERROR_UNICODE_START) != NULL);
    assert(
        passgen_token_state_string(PASSGEN_TOKEN_ERROR_UNICODE_PAYLOAD) !=
        NULL);
    assert(
        passgen_token_state_string(PASSGEN_TOKEN_ERROR_UNICODE_PAYLOAD_LEN) !=
        NULL);

    // undefined
    assert(
        passgen_token_state_string(PASSGEN_TOKEN_UNICODE_PAYLOAD + 1) == NULL);
    assert(
        passgen_token_state_string(
            PASSGEN_TOKEN_ERROR_UNICODE_PAYLOAD_LEN - 1) == NULL);

    return test_ok;
}

// Test that the token parser correctly keeps track of byte and codepoint
// offsets.
test_result test_token_normal_offsets(void) {
    struct passgen_token_parser parser = {0};
    struct passgen_token token = {0};

#define PARSE(cp, width)                                   \
    assert(                                                \
        passgen_token_parse(&parser, &token, width, cp) == \
        PASSGEN_TOKEN_INIT);                               \
    assert(parser.state == PASSGEN_TOKEN_INIT);            \
    assert(token.codepoint == cp);

    PARSE('a', 1);
    assert(parser.offset == 1);
    assert(parser.byte_offset == 1);
    assert(token.offset == 0);
    assert(token.byte_offset == 0);

    PARSE('b', 1);
    assert(parser.offset == 2);
    assert(parser.byte_offset == 2);
    assert(token.offset == 1);
    assert(token.byte_offset == 1);

    PARSE('c', 2);
    assert(parser.offset == 3);
    assert(parser.byte_offset == 4);
    assert(token.offset == 2);
    assert(token.byte_offset == 2);

    PARSE(' ', 2);
    assert(parser.offset == 4);
    assert(parser.byte_offset == 6);
    assert(token.offset == 3);
    assert(token.byte_offset == 4);

    PARSE('!', 3);
    assert(parser.offset == 5);
    assert(parser.byte_offset == 9);
    assert(token.offset == 4);
    assert(token.byte_offset == 6);

    PARSE('[', 3);
    assert(parser.offset == 6);
    assert(parser.byte_offset == 12);
    assert(token.offset == 5);
    assert(token.byte_offset == 9);

    PARSE(']', 4);
    assert(parser.offset == 7);
    assert(parser.byte_offset == 16);
    assert(token.offset == 6);
    assert(token.byte_offset == 12);

#undef PARSE

    return test_ok;
}

// Test that the token parser correctly keeps track of byte and codepoint
// offsets.
test_result test_token_multi_offsets(void) {
    struct passgen_token_parser parser = {0};
    struct passgen_token token = {0};

    assert(
        passgen_token_parse(&parser, &token, 1, '\\') == PASSGEN_TOKEN_ESCAPED);
    assert(passgen_token_parse(&parser, &token, 1, '[') == PASSGEN_TOKEN_INIT);
    assert(parser.offset == 2);
    assert(parser.byte_offset == 2);
    assert(token.offset == 0);
    assert(token.byte_offset == 0);

    assert(
        passgen_token_parse(&parser, &token, 1, '\\') == PASSGEN_TOKEN_ESCAPED);
    assert(passgen_token_parse(&parser, &token, 1, ']') == PASSGEN_TOKEN_INIT);
    assert(parser.offset == 4);
    assert(parser.byte_offset == 4);
    assert(token.offset == 2);
    assert(token.byte_offset == 2);

    assert(
        passgen_token_parse(&parser, &token, 1, '\\') == PASSGEN_TOKEN_ESCAPED);
    assert(
        passgen_token_parse(&parser, &token, 1, 'u') == PASSGEN_TOKEN_UNICODE);
    assert(
        passgen_token_parse(&parser, &token, 1, '{') ==
        PASSGEN_TOKEN_UNICODE_PAYLOAD);
    assert(
        passgen_token_parse(&parser, &token, 1, '0') ==
        PASSGEN_TOKEN_UNICODE_PAYLOAD);
    assert(
        passgen_token_parse(&parser, &token, 1, 'a') ==
        PASSGEN_TOKEN_UNICODE_PAYLOAD);
    assert(passgen_token_parse(&parser, &token, 1, '}') == PASSGEN_TOKEN_INIT);
    assert(parser.offset == 10);
    assert(parser.byte_offset == 10);
    assert(token.offset == 4);
    assert(token.byte_offset == 4);

    return test_ok;
}
