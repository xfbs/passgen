#include <criterion/criterion.h>
#include <passgen/data/token.h>
#include <passgen/token.h>

static char hex_small[16] = "0123456789abcdef";
static char hex_big[16] = "0123456789ABCDEF";

// Test a normal token -- it should just pass right though the parser.
static void token_normal(int32_t codepoint) {
    struct passgen_token_parser parser = {0};
    struct passgen_token token = {0};

    cr_assert_eq(passgen_token_parse(&parser, &token, codepoint), PASSGEN_TOKEN_INIT);
    cr_assert_eq(parser.state, PASSGEN_TOKEN_INIT);
    cr_assert_eq(token.codepoint, codepoint);
}

// Test escaped token -- it should pass through the parser, but with the
// escaped flag set.
static void token_escaped(int32_t codepoint, int32_t output) {
    struct passgen_token_parser parser = {0};
    struct passgen_token token = {0};

    cr_assert_eq(passgen_token_parse(&parser, &token, '\\'), PASSGEN_TOKEN_ESCAPED);
    cr_assert_eq(parser.state, PASSGEN_TOKEN_ESCAPED);
    cr_assert_eq(passgen_token_parse(&parser, &token, codepoint), PASSGEN_TOKEN_INIT);
    cr_assert_eq(parser.state, PASSGEN_TOKEN_INIT);
    cr_assert_eq(token.codepoint, output);
}

// Test unicode token.
static void token_unicode(size_t len, int32_t cps[], int32_t output) {
    struct passgen_token_parser parser = {0};
    struct passgen_token token = {0};

    cr_assert_eq(passgen_token_parse(&parser, &token, '\\'), PASSGEN_TOKEN_ESCAPED);
    cr_assert_eq(parser.state, PASSGEN_TOKEN_ESCAPED);
    cr_assert_eq(passgen_token_parse(&parser, &token, 'u'), PASSGEN_TOKEN_UNICODE);
    cr_assert_eq(parser.state, PASSGEN_TOKEN_UNICODE);
    cr_assert_eq(passgen_token_parse(&parser, &token, '{'), PASSGEN_TOKEN_UNICODE_PAYLOAD);
    cr_assert_eq(parser.state, PASSGEN_TOKEN_UNICODE_PAYLOAD);
    cr_assert_eq(parser.data.unicode_payload.length, 0);
    cr_assert_eq(parser.data.unicode_payload.codepoint, 0);

    for(size_t i = 0; i < len; i++) {
        cr_assert_eq(passgen_token_parse(&parser, &token, cps[i]), PASSGEN_TOKEN_UNICODE_PAYLOAD);
        cr_assert_eq(parser.data.unicode_payload.length, i + 1);
    }

    cr_assert_eq(passgen_token_parse(&parser, &token, '}'), PASSGEN_TOKEN_INIT);
    cr_assert_eq(parser.state, PASSGEN_TOKEN_INIT);
    cr_assert_eq(token.codepoint, output);
}

// Test that a specific error is generated after parsing the sequence.
static void token_error(size_t len, int32_t cps[], int ret) {
    struct passgen_token_parser parser = {0};
    struct passgen_token token = {0};

    for(size_t i = 0; i < (len - 1); i++) {
        cr_assert(passgen_token_parse(&parser, &token, cps[i]) > 0);
    }

    cr_assert_eq(passgen_token_parse(&parser, &token, cps[len - 1]), ret);
    cr_assert_eq(parser.state, ret);
}

Test(token, normal) {
    token_normal('a');
    token_normal('b');
    token_normal('c');
    token_normal(' ');
    token_normal('!');
    token_normal('[');
    token_normal(']');
}

Test(token, escaped) {
    token_escaped('\\', '\\');
    token_escaped('a', '\a');
    token_escaped('b', '\b');
    token_escaped('f', '\f');
    token_escaped('r', '\r');
    token_escaped('n', '\n');
    token_escaped('e', '\033');
}

Test(token, unicode_short) {
    for(size_t cp1 = 0; cp1 < 16; cp1++) {
        for(size_t cp2 = 0; cp2 < 16; cp2++) {
            int32_t result = (cp1 << 4) + cp2;
            token_unicode(2, (int32_t[]){hex_small[cp1], hex_small[cp2]}, result);
            token_unicode(2, (int32_t[]){hex_small[cp1], hex_big[cp2]}, result);
            token_unicode(2, (int32_t[]){hex_big[cp1], hex_small[cp2]}, result);
            token_unicode(2, (int32_t[]){hex_big[cp1], hex_big[cp2]}, result);
        }
    }
}

Test(token, unicode_long) {
    token_unicode(0, (int32_t []){}, 0x00);
    token_unicode(6, (int32_t []){'0', '1', 'F', '6', '4', '2'}, 0x1F642);
    token_unicode(6, (int32_t []){'0', '1', 'f', '6', '0', 'b'}, 0x1F60B);
    token_unicode(6, (int32_t []){'0', '0', '0', '0', 'f', 'c'}, 0xFC);
}

Test(token, unicode_error_length) {
    token_error(10, (int32_t []){'\\', 'u', '{', '0', '0', '0', '0', '0', '0', '0'}, PASSGEN_TOKEN_ERROR_UNICODE_PAYLOAD_LEN);
    token_error(10, (int32_t []){'\\', 'u', '{', '9', 'a', 'b', 'c', 'd', 'e', 'f'}, PASSGEN_TOKEN_ERROR_UNICODE_PAYLOAD_LEN);
}

Test(token, unicode_error_char) {
    token_error(4, (int32_t []){'\\', 'u', '{', 'g'}, PASSGEN_TOKEN_ERROR_UNICODE_PAYLOAD);
    token_error(4, (int32_t []){'\\', 'u', '{', '-'}, PASSGEN_TOKEN_ERROR_UNICODE_PAYLOAD);
    token_error(6, (int32_t []){'\\', 'u', '{', '0', '1', '{'}, PASSGEN_TOKEN_ERROR_UNICODE_PAYLOAD);
}

Test(token, unicode_error_start) {
    token_error(3, (int32_t []){'\\', 'u', '0'}, PASSGEN_TOKEN_ERROR_UNICODE_START);
    token_error(3, (int32_t []){'\\', 'u', 'a'}, PASSGEN_TOKEN_ERROR_UNICODE_START);
    token_error(3, (int32_t []){'\\', 'u', '4'}, PASSGEN_TOKEN_ERROR_UNICODE_START);
    token_error(3, (int32_t []){'\\', 'u', 'g'}, PASSGEN_TOKEN_ERROR_UNICODE_START);
}
