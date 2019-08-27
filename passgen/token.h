#pragma once
#include "unicode.h"

/// Represents a valid substring of a string.
struct pattern_substring {
  size_t offset;
  size_t length;
};

enum pattern_token_type {
  PATTERN_TOKEN_EOF,
  PATTERN_TOKEN_ERROR,
  PATTERN_TOKEN_REGULAR,
  PATTERN_TOKEN_SPECIAL,
  PATTERN_TOKEN_UNICODE,
  PATTERN_TOKEN_ESCAPED,
};

struct pattern_token {
  enum pattern_token_type type;
  int32_t codepoint;
  struct pattern_substring data;
};

/// Parse the next token, without advancing the unicode reader.
struct pattern_token pattern_token_peek(const struct unicode_iter *iter);

/// Parse the next token, advancing the unicode reader.
struct pattern_token pattern_token_next(struct unicode_iter *iter);
