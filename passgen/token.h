#pragma once
#include "unicode.h"

/// Represents a valid substring of a string.
struct passgen_substring {
  size_t offset;
  size_t length;
};

enum passgen_token_type {
  PATTERN_TOKEN_EOF,
  PATTERN_TOKEN_ERROR,
  PATTERN_TOKEN_REGULAR,
  PATTERN_TOKEN_SPECIAL,
  PATTERN_TOKEN_UNICODE,
  PATTERN_TOKEN_ESCAPED,
};

struct passgen_token {
  enum passgen_token_type type;
  int32_t codepoint;
  struct passgen_substring data;
};

typedef struct passgen_token passgen_token_t;

/// Parse the next token, without advancing the unicode reader.
struct passgen_token passgen_token_peek(const unicode_iter_t *iter);

/// Parse the next token, advancing the unicode reader.
struct passgen_token passgen_token_next(unicode_iter_t *iter);
