#include "passgen/unicode.h"
#include "tests.h"

test_result test_unicode_iter_sized(void) {
  struct unicode_iter iter;

  iter = unicode_iter("abc");
  assert(iter.length == 3);

  iter = unicode_iter_sized("abcdef", 5);
  assert(iter.length == 5);

  return test_ok;
}

test_result test_unicode_iter_peek(void) {
  struct unicode_iter iter;
  struct unicode_iter_result result;
  const unsigned char data[] = {195, 188, 195, 159, 0};

  iter = unicode_iter(data);
  assert(iter.pos == 0);
  assert(iter.length == 4);

  result = unicode_iter_peek(&iter);
  assert(result.ok);
  assert(iter.pos == 0);
  assert(iter.length == 4);
  assert(result.error == 2);
  assert(result.codepoint = 0xE4);

  iter.pos = 2;
  result = unicode_iter_peek(&iter);
  assert(result.ok);
  assert(iter.pos == 2);
  assert(result.error == 2);
  assert(result.codepoint = 0xDF);

  iter.pos = 4;
  result = unicode_iter_peek(&iter);
  assert(!result.ok);
  assert(result.error == 0);
  assert(result.codepoint == 0);

  return test_ok;
}

test_result test_unicode_iter_error(void) {
  struct unicode_iter iter;
  struct unicode_iter_result result;

  iter = unicode_iter("\xc3\x28");
  result = unicode_iter_next(&iter);
  assert(!result.ok);

  iter = unicode_iter("\xe2\x28\xa1");
  result = unicode_iter_next(&iter);
  assert(!result.ok);

  iter = unicode_iter("\xe2\x82\x28");
  result = unicode_iter_next(&iter);
  assert(!result.ok);

  return test_ok;
}

test_result test_unicode_iter_next(void) {
  struct unicode_iter iter;
  struct unicode_iter_result result;
  const char *data = "abcd";

  iter = unicode_iter(data);
  assert(iter.pos == 0);
  assert(iter.length == 4);

  result = unicode_iter_next(&iter);
  assert(result.ok);
  assert(result.codepoint == 'a');

  result = unicode_iter_next(&iter);
  assert(result.ok);
  assert(result.codepoint == 'b');

  result = unicode_iter_next(&iter);
  assert(result.ok);
  assert(result.codepoint == 'c');

  result = unicode_iter_next(&iter);
  assert(result.ok);
  assert(result.codepoint == 'd');

  result = unicode_iter_next(&iter);
  assert(!result.ok);
  assert(result.codepoint == 0);

  return test_ok;
}
