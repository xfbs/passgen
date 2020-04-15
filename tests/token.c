#include <stddef.h>

#include "passgen/token_old.h"
#include "tests.h"
#include "passgen/unicode.h"

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
  const char parsed[] =
      {'\a', '\b', '\033', '\f', '\n', '\r', '\t', '\v', '\\'};

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

  const char *unichars = "\\u{FC}\\u{B5}\\u{3f4}\\u{0}";
  iter = unicode_iter(unichars);

  token = passgen_token_next(&iter);
  assert(!passgen_token_is_error(&token));
  assert(passgen_token_is_normal(&token));
  assert(passgen_token_is_unicode(&token));
  assert(!passgen_token_is_regular(&token));
  assert(!passgen_token_is_eof(&token));
  assert(token.type == PATTERN_TOKEN_UNICODE);
  assert(token.codepoint == 0xFC);
  assert(token.pos.offset == 0);
  assert(token.pos.length == 6);
  assert(token.data.offset == 3);
  assert(token.data.length == 2);

  token = passgen_token_next(&iter);
  assert(!passgen_token_is_error(&token));
  assert(passgen_token_is_normal(&token));
  assert(passgen_token_is_unicode(&token));
  assert(!passgen_token_is_regular(&token));
  assert(!passgen_token_is_eof(&token));
  assert(token.type == PATTERN_TOKEN_UNICODE);
  assert(token.codepoint == 0xB5);
  assert(token.pos.offset == 6);
  assert(token.pos.length == 6);
  assert(token.data.offset == 9);
  assert(token.data.length == 2);

  token = passgen_token_next(&iter);
  assert(!passgen_token_is_error(&token));
  assert(passgen_token_is_normal(&token));
  assert(passgen_token_is_unicode(&token));
  assert(!passgen_token_is_regular(&token));
  assert(!passgen_token_is_eof(&token));
  assert(token.type == PATTERN_TOKEN_UNICODE);
  assert(token.codepoint == 0x3F4);
  assert(token.pos.offset == 12);
  assert(token.pos.length == 7);
  assert(token.data.offset == 15);
  assert(token.data.length == 3);

  token = passgen_token_next(&iter);
  assert(!passgen_token_is_error(&token));
  assert(passgen_token_is_normal(&token));
  assert(passgen_token_is_unicode(&token));
  assert(!passgen_token_is_regular(&token));
  assert(!passgen_token_is_eof(&token));
  assert(token.type == PATTERN_TOKEN_UNICODE);
  assert(token.codepoint == 0);
  assert(token.pos.offset == 19);
  assert(token.pos.length == 5);
  assert(token.data.offset == 22);
  assert(token.data.length == 1);

  unichars = "\\(\\)\\[\\]\\{\\}\\|";
  for(iter = unicode_iter(unichars); iter.pos < 14;) {
    token = passgen_token_next(&iter);
    assert(!passgen_token_is_error(&token));
    assert(passgen_token_is_escaped(&token));
    assert(!passgen_token_is_eof(&token));
    assert(token.codepoint == unichars[iter.pos - 1]);
    assert(token.pos.offset == (iter.pos - 2));
    assert(token.pos.length == 2);
  }

  return test_ok;
}

test_result test_passgen_token_errors(void) {
  struct unicode_iter iter;
  passgen_token_t token;

  // test EOF
  iter = unicode_iter(" ");

  token = passgen_token_next(&iter);
  assert(passgen_token_is_regular(&token));
  assert(token.codepoint == ' ');

  token = passgen_token_next(&iter);
  assert(!passgen_token_is_normal(&token));
  assert(!passgen_token_is_error(&token));
  assert(passgen_token_is_eof(&token));
  assert(token.type == PATTERN_TOKEN_EOF);
  assert(token.pos.offset == 1);
  assert(token.pos.length == 0);

  // illegal escape
  iter = unicode_iter(" \\m");

  token = passgen_token_next(&iter);
  assert(passgen_token_is_regular(&token));
  assert(token.codepoint == ' ');

  token = passgen_token_next(&iter);
  assert(!passgen_token_is_normal(&token));
  assert(passgen_token_is_error(&token));
  assert(!passgen_token_is_eof(&token));
  assert(token.type == PASSGEN_TOKEN_ERROR_ESCAPE);
  assert(token.pos.offset == 1);
  assert(token.pos.length == 2);
  assert(token.error == 2);

  // lbrace
  iter = unicode_iter(" \\u[x]");

  token = passgen_token_next(&iter);
  assert(passgen_token_is_regular(&token));
  assert(token.codepoint == ' ');

  token = passgen_token_next(&iter);
  assert(!passgen_token_is_normal(&token));
  assert(passgen_token_is_error(&token));
  assert(!passgen_token_is_eof(&token));
  assert(token.type == PASSGEN_TOKEN_ERROR_LBRACE);
  assert(token.pos.offset == 1);
  assert(token.pos.length == 3);
  assert(token.error == 3);

  // hex
  iter = unicode_iter(" \\u{axb}");

  token = passgen_token_next(&iter);
  assert(passgen_token_is_regular(&token));
  assert(token.codepoint == ' ');

  token = passgen_token_next(&iter);
  assert(!passgen_token_is_normal(&token));
  assert(passgen_token_is_error(&token));
  assert(!passgen_token_is_eof(&token));
  assert(token.type == PASSGEN_TOKEN_ERROR_UNICODE_HEX);
  assert(token.pos.offset == 1);
  assert(token.pos.length == 5);
  assert(token.error == 5);

  // char out of range
  iter = unicode_iter(" \\u{110000}");

  token = passgen_token_next(&iter);
  assert(passgen_token_is_regular(&token));
  assert(token.codepoint == ' ');

  token = passgen_token_next(&iter);
  assert(!passgen_token_is_normal(&token));
  assert(passgen_token_is_error(&token));
  assert(!passgen_token_is_eof(&token));
  assert(token.type == PASSGEN_TOKEN_ERROR_UNICODE_CHAR);
  assert(token.pos.offset == 1);
  assert(token.pos.length == 10);
  assert(token.error == 4);

  // char out of range
  iter = unicode_iter(" \\u{d8f3}");

  token = passgen_token_next(&iter);
  assert(passgen_token_is_regular(&token));
  assert(token.codepoint == ' ');

  token = passgen_token_next(&iter);
  assert(!passgen_token_is_normal(&token));
  assert(passgen_token_is_error(&token));
  assert(!passgen_token_is_eof(&token));
  assert(token.type == PASSGEN_TOKEN_ERROR_UNICODE_CHAR);
  assert(token.pos.offset == 1);
  assert(token.pos.length == 8);
  assert(token.error == 4);

  // hex too long
  iter = unicode_iter(" \\u{000000f6}");

  token = passgen_token_next(&iter);
  assert(passgen_token_is_regular(&token));
  assert(token.codepoint == ' ');

  token = passgen_token_next(&iter);
  assert(!passgen_token_is_normal(&token));
  assert(passgen_token_is_error(&token));
  assert(!passgen_token_is_eof(&token));
  assert(token.type == PASSGEN_TOKEN_ERROR_UNICODE_CHAR);
  assert(token.pos.offset == 1);
  assert(token.pos.length == 10);
  assert(token.error == 4);

  // hex too long
  iter = unicode_iter(" \xc3\x28");

  token = passgen_token_next(&iter);
  assert(passgen_token_is_regular(&token));
  assert(token.codepoint == ' ');

  token = passgen_token_next(&iter);
  assert(!passgen_token_is_normal(&token));
  assert(passgen_token_is_error(&token));
  assert(!passgen_token_is_eof(&token));
  assert(token.type >= PASSGEN_TOKEN_ERROR_UTF8);
  assert(token.pos.offset == 1);
  assert(token.pos.length == 0);
  assert(token.error == 1);

  // hex too long
  iter = unicode_iter(" \\\xc3\x28");

  token = passgen_token_next(&iter);
  assert(passgen_token_is_regular(&token));
  assert(token.codepoint == ' ');

  token = passgen_token_next(&iter);
  assert(!passgen_token_is_normal(&token));
  assert(passgen_token_is_error(&token));
  assert(!passgen_token_is_eof(&token));
  assert(token.type >= PASSGEN_TOKEN_ERROR_UTF8);
  assert(token.pos.offset == 1);
  assert(token.pos.length == 1);
  assert(token.error == 2);

  // no data
  iter = unicode_iter(" \\u{}");

  token = passgen_token_next(&iter);
  assert(passgen_token_is_regular(&token));
  assert(token.codepoint == ' ');

  token = passgen_token_next(&iter);
  assert(!passgen_token_is_normal(&token));
  assert(passgen_token_is_error(&token));
  assert(!passgen_token_is_eof(&token));
  assert(token.type == PASSGEN_TOKEN_ERROR_UNICODE_EMPTY);
  assert(token.pos.offset == 1);
  assert(token.pos.length == 4);
  assert(token.data.offset == 4);
  assert(token.data.length == 0);
  assert(token.error == 4);

  // special
  iter = unicode_iter(" \\p");

  token = passgen_token_next(&iter);
  assert(passgen_token_is_regular(&token));
  assert(token.codepoint == ' ');

  token = passgen_token_next(&iter);
  assert(!passgen_token_is_normal(&token));
  assert(!passgen_token_is_regular(&token));
  assert(!passgen_token_is_error(&token));
  assert(passgen_token_is_special(&token));
  assert(token.type == PATTERN_TOKEN_SPECIAL);
  assert(token.pos.offset == 1);
  assert(token.pos.length == 2);
  assert(token.codepoint == 'p');

  return test_ok;
}
