#pragma once
#include "passgen/enum_mapping.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

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

enum passgen_token_type {
  PASSGEN_TOKEN_NORMAL,
  PASSGEN_TOKEN_SPECIAL,
  PASSGEN_TOKEN_ESCAPED_TYPE,
  PASSGEN_TOKEN_ESCAPED_SIMPLE_TYPE,
  PASSGEN_TOKEN_ESCAPED_PASSTHRU,
  PASSGEN_TOKEN_ESCAPED_UNICODE
};

enum passgen_token_escaped {
  PASSGEN_TOKEN_ESCAPED_NOT,
  PASSGEN_TOKEN_ESCAPED_SIMPLE,
  PASSGEN_TOKEN_ESCPAED_NORMAL
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
  enum passgen_token_type type;
  bool escaped;
  bool normal_escaped;
};

PASSGEN_ENUM_MAPPING(passgen_token_state);
PASSGEN_ENUM_MAPPING(passgen_token_type);
PASSGEN_ENUM_MAPPING(passgen_token_escaped);
