#pragma once
#include "passgen/enum_mapping.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PASSGEN_TOKEN_ESCAPED_BIT (1 << 30)
#define PASSGEN_TOKEN_UNICODE_MASK (1 << 21 - 1)

// when updating any of the enums, make sure to also update the mappings.
enum passgen_token_state {
  PASSGEN_TOKEN_INIT = 0,
  PASSGEN_TOKEN_ESCAPED,
  PASSGEN_TOKEN_UNICODE,
  PASSGEN_TOKEN_UNICODE_PAYLOAD,

  // errors
  PASSGEN_TOKEN_ERROR_UNICODE_START = -1,
  PASSGEN_TOKEN_ERROR_UNICODE_PAYLOAD = -2,
  PASSGEN_TOKEN_ERROR_UNICODE_PAYLOAD_LEN = -3
};

struct passgen_token_parser {
  enum passgen_token_state state;
  union {
    // when parsing a \u{FA} unicode literal, this is where we keep the
    // state.
    struct {
      size_t length;
      uint32_t codepoint;
    } unicode_payload;
  } data;
};

struct passgen_token {
  uint32_t codepoint;
  void *origin;
};

void passgen_token_parser_init(struct passgen_token_parser *token_parser);

PASSGEN_ENUM_MAPPING(passgen_token_state);
