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

#define SEED 320843200

#define PREAMBLE()                                \
  struct passgen_parser parser;                   \
  struct passgen_token_parser token_parser = {0}; \
  struct passgen_token token = {0};               \
  passgen_random_t random;                        \
  passgen_parser_init(&parser);                   \
  assert(0 == passgen_parse_start(&parser));      \
  assert(passgen_random_open_xorshift(&random, SEED))

#define GENERATE(output, pattern)                                 \
  do {                                                            \
    passgen_parser_init(&parser);                                 \
    assert(0 == passgen_parse_start(&parser));                    \
    passgen_token_parser_init(&token_parser);                     \
                                                                  \
    int token_parser_state;                                       \
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
    passgen_generate_fill(                                        \
        &parser.pattern,                                          \
        &random,                                                  \
        NULL,                                                     \
        output,                                                   \
        sizeof(output));                                          \
                                                                  \
    passgen_parser_free(&parser);                                 \
  } while(0)

#define POSTAMBLE() passgen_random_close(&random)

test_result test_generate_empty(void) {
  PREAMBLE();
  char output[10];
  const char *pattern = "";
  GENERATE(output, pattern);

  POSTAMBLE();

  return test_ok;
}
