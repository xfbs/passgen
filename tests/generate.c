#include "passgen/parser.h"

#include "passgen/container/stack/range.h"
#include "passgen/container/stack/segment.h"
#include "passgen/container/stack/segment_item.h"
#include "passgen/data/char.h"
#include "passgen/data/group.h"
#include "passgen/data/parser.h"
#include "passgen/data/pattern.h"
#include "passgen/data/pattern_kind.h"
#include "passgen/data/range.h"
#include "passgen/data/repeat.h"
#include "passgen/data/segment.h"
#include "passgen/data/segment_item.h"
#include "passgen/data/set.h"
#include "passgen/data/token.h"
#include "passgen/generate.h"
#include "passgen/token.h"
#include "tests.h"

// don't change this seed - that will break tests!
#define SEED 320843200

#define PREAMBLE()                                \
  struct passgen_parser parser;                   \
  struct passgen_token_parser token_parser = {0}; \
  struct passgen_token token = {0};               \
  const char *pattern;                            \
  passgen_random_t random;                        \
  assert(passgen_random_open_xorshift(&random, SEED))

#define GENERATE(output, pattern)                                 \
  do {                                                            \
    passgen_parser_init(&parser);                                 \
    assert(0 == passgen_parse_start(&parser));                    \
    passgen_token_parser_init(&token_parser);                     \
                                                                  \
    int token_parser_state = PASSGEN_TOKEN_INIT;                  \
    for(size_t i = 0; pattern[i]; i++) {                          \
      token_parser_state =                                        \
          passgen_token_parse(&token_parser, &token, pattern[i]); \
      if(token_parser_state == PASSGEN_TOKEN_INIT) {              \
        assert(0 == passgen_parse_token(&parser, &token));        \
      }                                                           \
    }                                                             \
                                                                  \
    assert(token_parser_state == PASSGEN_TOKEN_INIT);             \
    assert(0 == passgen_parse_finish(&parser));                   \
    size_t len = passgen_generate_fill_unicode(                   \
        &parser.pattern,                                          \
        &random,                                                  \
        NULL,                                                     \
        output,                                                   \
        sizeof(output));                                          \
    output[len] = 0;                                              \
                                                                  \
    passgen_parser_free(&parser);                                 \
  } while(0)

#define POSTAMBLE() passgen_random_close(&random)

test_result test_generate_empty(void) {
  PREAMBLE();
  int32_t output[10];

  pattern = "";
  GENERATE(output, pattern);
  assert(output[0] == '\0');

  POSTAMBLE();
  return test_ok;
}

test_result test_generate_chars(void) {
  PREAMBLE();
  int32_t output[10];

  // single char
  pattern = "a";
  GENERATE(output, pattern);
  assert(output[0] == 'a');
  assert(output[1] == '\0');

  // multiple chars
  pattern = "abc";
  GENERATE(output, pattern);
  assert(output[0] == 'a');
  assert(output[1] == 'b');
  assert(output[2] == 'c');
  assert(output[3] == '\0');

  // escaped chars
  pattern = "pass\\[\\{\\u{78}";
  GENERATE(output, pattern);
  assert(output[0] == 'p');
  assert(output[1] == 'a');
  assert(output[2] == 's');
  assert(output[3] == 's');
  assert(output[4] == '[');
  assert(output[5] == '{');
  assert(output[6] == 'x');
  assert(output[7] == '\0');

  POSTAMBLE();
  return test_ok;
}

test_result test_generate_segments(void) {
  PREAMBLE();
  int32_t output[10];

  // either a or b
  pattern = "a|b";
  GENERATE(output, pattern);
  assert(output[0] == 'a');
  assert(output[1] == '\0');

  passgen_random_u64(&random);

  GENERATE(output, pattern);
  assert(output[0] == 'b');
  assert(output[1] == '\0');

  POSTAMBLE();
  return test_ok;
}

test_result test_generate_group(void) {
  PREAMBLE();
  int32_t output[10];

  // "abc"
  pattern = "(abc)";
  GENERATE(output, pattern);
  assert(output[0] == 'a');
  assert(output[1] == 'b');
  assert(output[2] == 'c');
  assert(output[3] == '\0');

  // either "a" or "b"
  pattern = "(a|b)";
  GENERATE(output, pattern);
  assert(output[0] == 'a');
  assert(output[1] == '\0');

  passgen_random_u64(&random);
  passgen_random_u64(&random);

  GENERATE(output, pattern);
  assert(output[0] == 'b');
  assert(output[1] == '\0');

  POSTAMBLE();
  return test_ok;
}

test_result test_generate_set(void) {
  PREAMBLE();
  int32_t output[10];

  // "abc"
  pattern = "[a]";
  GENERATE(output, pattern);
  assert(output[0] == 'a');
  assert(output[1] == '\0');

  // "a" or "b" or "c"
  pattern = "[abc]";
  GENERATE(output, pattern);
  assert(output[0] == 'c');
  assert(output[1] == '\0');

  passgen_random_u64(&random);
  GENERATE(output, pattern);
  assert(output[0] == 'a');
  assert(output[1] == '\0');

  passgen_random_u64(&random);
  passgen_random_u64(&random);
  passgen_random_u64(&random);
  GENERATE(output, pattern);
  assert(output[0] == 'b');
  assert(output[1] == '\0');

  pattern = "[a-c]";
  GENERATE(output, pattern);
  assert(output[0] == 'c');
  assert(output[1] == '\0');

  GENERATE(output, pattern);
  assert(output[0] == 'b');
  assert(output[1] == '\0');

  passgen_random_u64(&random);
  passgen_random_u64(&random);
  passgen_random_u64(&random);
  passgen_random_u64(&random);
  GENERATE(output, pattern);
  assert(output[0] == 'a');
  assert(output[1] == '\0');

  POSTAMBLE();
  return test_ok;
}

test_result test_generate_maybe(void) {
  PREAMBLE();
  int32_t output[10];

  // char or not?
  pattern = "a?";
  GENERATE(output, pattern);
  assert(output[0] == 'a');
  assert(output[1] == '\0');

  passgen_random_u64(&random);

  GENERATE(output, pattern);
  assert(output[0] == '\0');

  // group or not?
  pattern = "(b)?";
  GENERATE(output, pattern);
  assert(output[0] == '\0');

  passgen_random_u64(&random);

  GENERATE(output, pattern);
  assert(output[0] == 'b');
  assert(output[1] == '\0');

  // set or not?
  pattern = "[c]?";
  GENERATE(output, pattern);
  assert(output[0] == '\0');

  passgen_random_u64(&random);

  GENERATE(output, pattern);
  assert(output[0] == 'c');
  assert(output[1] == '\0');

  POSTAMBLE();
  return test_ok;
}

test_result test_generate_repeat(void) {
  PREAMBLE();
  int32_t output[10];

  // char or not?
  pattern = "a{5}";
  GENERATE(output, pattern);
  assert(output[0] == 'a');
  assert(output[1] == 'a');
  assert(output[2] == 'a');
  assert(output[3] == 'a');
  assert(output[4] == 'a');
  assert(output[5] == '\0');

  pattern = "b{2,3}";
  GENERATE(output, pattern);
  assert(output[0] == 'b');
  assert(output[1] == 'b');
  assert(output[2] == '\0');

  passgen_random_u64(&random);
  GENERATE(output, pattern);
  assert(output[0] == 'b');
  assert(output[1] == 'b');
  assert(output[2] == 'b');
  assert(output[3] == '\0');

  pattern = "[abc]{3}";
  GENERATE(output, pattern);
  assert(output[0] == 'a');
  assert(output[1] == 'a');
  assert(output[2] == 'c');
  assert(output[3] == '\0');

  pattern = "(ab|x){3}";
  GENERATE(output, pattern);
  assert(output[0] == 'x');
  assert(output[1] == 'x');
  assert(output[2] == 'a');
  assert(output[3] == 'b');
  assert(output[4] == '\0');

  POSTAMBLE();
  return test_ok;
}
