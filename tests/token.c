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
