#include "passgen/token.h"
#include "tests/tests.h"

test_result test_passgen_token_next(void) {
    struct unicode_iter iter;
    passgen_token_t token;

    iter = unicode_iter("a");

    token = passgen_token_next(&iter);
    assert(token.type == PATTERN_TOKEN_REGULAR);
    assert(token.codepoint == 'a');

    token = passgen_token_next(&iter);
    assert(token.type == PATTERN_TOKEN_EOF);

    iter = unicode_iter("ä");

    token = passgen_token_next(&iter);
    assert(token.type == PATTERN_TOKEN_REGULAR);
    assert(token.codepoint == 0xE4);

    token = passgen_token_next(&iter);
    assert(token.type == PATTERN_TOKEN_EOF);

    return test_ok;
}

test_result test_passgen_token_peek(void) {
    struct unicode_iter iter;
    passgen_token_t token;

    iter = unicode_iter("a");

    token = passgen_token_peek(&iter);
    assert(token.type == PATTERN_TOKEN_REGULAR);
    assert(token.codepoint == 'a');

    token = passgen_token_peek(&iter);
    assert(token.type == PATTERN_TOKEN_REGULAR);
    assert(token.codepoint == 'a');

    iter = unicode_iter("ä");

    token = passgen_token_peek(&iter);
    assert(token.type == PATTERN_TOKEN_REGULAR);
    assert(token.codepoint == 0xE4);

    token = passgen_token_peek(&iter);
    assert(token.type == PATTERN_TOKEN_REGULAR);
    assert(token.codepoint == 0xE4);

    return test_ok;
}

test_result test_passgen_token_types(void) {
    struct unicode_iter iter;
    passgen_token_t token;

    // test escape token types
    iter = unicode_iter("\\a\\b\\e\\f\\n\\r\\t\\v\\\\");
    const char parsed[] = {'\a', '\b', '\033', '\f', '\n', '\r', '\t', '\v', '\\'};

    for(size_t i = 0; i < sizeof(parsed); i++) {
        token = passgen_token_next(&iter);

        // check it's the right type
        assert(!passgen_token_is_error(&token));
        assert(passgen_token_is_normal(&token));
        assert(passgen_token_is_escaped(&token));
        assert(!passgen_token_is_eof(&token));
        assert(token.type == PATTERN_TOKEN_ESCAPED);
        assert(token.codepoint == parsed[i]);

        // check pos and len
        assert(token.pos.offset == 2 * i);
        assert(token.pos.length == 2);
    }

    // check eof type
    token = passgen_token_next(&iter);
    assert(!passgen_token_is_normal(&token));
    assert(!passgen_token_is_error(&token));
    assert(passgen_token_is_eof(&token));

    // check eof pos and len
    assert(token.pos.offset == 2 * sizeof(parsed));
    assert(token.pos.length == 0);

    const char *chars = "abcdefghijklmnopqrstuvwzyx1234567&^%$#@!";
    iter = unicode_iter(chars);

    for(size_t i = 0; chars[i]; i++) {
        token = passgen_token_next(&iter);

        // check type
        assert(!passgen_token_is_error(&token));
        assert(passgen_token_is_normal(&token));
        assert(passgen_token_is_regular(&token));
        assert(!passgen_token_is_eof(&token));
        assert(token.type == PATTERN_TOKEN_REGULAR);
        assert(token.codepoint == chars[i]);

        // check pos and len
        assert(token.pos.offset == i);
        assert(token.pos.length == 1);
    }

    token = passgen_token_next(&iter);
    assert(!passgen_token_is_normal(&token));
    assert(!passgen_token_is_error(&token));
    assert(passgen_token_is_eof(&token));

    const char *unichars = "\\u{FC}\\u{B5}\\u{3f4}";
    iter = unicode_iter(unichars);

    token = passgen_token_next(&iter);

    // check typ
    assert(!passgen_token_is_error(&token));
    assert(passgen_token_is_normal(&token));
    assert(passgen_token_is_unicode(&token));
    assert(!passgen_token_is_regular(&token));
    assert(!passgen_token_is_eof(&token));
    assert(token.type == PATTERN_TOKEN_UNICODE);
    assert(token.codepoint == 0xFC);
    assert(token.pos.offset == 0);
    assert(token.pos.length == 6);

    return test_ok;
}
