#include "passgen/token.h"
#include "tests/tests.h"

test_result test_pattern_token_next(void) {
  struct unicode_iter iter;
  struct pattern_token token;

  iter = unicode_iter("a");

  token = pattern_token_next(&iter);
  assert(token.type == PATTERN_TOKEN_REGULAR);
  assert(token.codepoint == 'a');

  token = pattern_token_next(&iter);
  assert(token.type == PATTERN_TOKEN_EOF);

  iter = unicode_iter("ä");

  token = pattern_token_next(&iter);
  assert(token.type == PATTERN_TOKEN_REGULAR);
  assert(token.codepoint == 0xE4);

  token = pattern_token_next(&iter);
  assert(token.type == PATTERN_TOKEN_EOF);

  return test_ok;
}

test_result test_pattern_token_peek(void) {
  struct unicode_iter iter;
  struct pattern_token token;

  iter = unicode_iter("a");

  token = pattern_token_peek(&iter);
  assert(token.type == PATTERN_TOKEN_REGULAR);
  assert(token.codepoint == 'a');

  token = pattern_token_peek(&iter);
  assert(token.type == PATTERN_TOKEN_REGULAR);
  assert(token.codepoint == 'a');

  iter = unicode_iter("ä");

  token = pattern_token_peek(&iter);
  assert(token.type == PATTERN_TOKEN_REGULAR);
  assert(token.codepoint == 0xE4);

  token = pattern_token_peek(&iter);
  assert(token.type == PATTERN_TOKEN_REGULAR);
  assert(token.codepoint == 0xE4);

  return test_ok;
}
