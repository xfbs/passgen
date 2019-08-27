#include "passgen/token.h"

struct pattern_token pattern_token_error(size_t pos) {
  return (struct pattern_token) {
    .type = PATTERN_TOKEN_ERROR,
    .data.offset = pos
  };
}

struct pattern_token pattern_token_eof(void) {
  return (struct pattern_token) {
    .type = PATTERN_TOKEN_EOF,
  };
}

struct pattern_token pattern_token_regular(int32_t codepoint) {
  return (struct pattern_token) {
    .type = PATTERN_TOKEN_REGULAR,
    .codepoint = codepoint
  };
}

struct pattern_token pattern_token_escaped(int32_t codepoint) {
  return (struct pattern_token) {
    .type = PATTERN_TOKEN_ESCAPED,
    .codepoint = codepoint
  };
}

/// Parse the next token, without advancing the unicode reader.
struct pattern_token pattern_token_peek(const struct unicode_iter *iter) {
  struct unicode_iter iter_copy = *iter;
  return pattern_token_next(&iter_copy);
}

struct pattern_token pattern_token_unicode(struct unicode_iter *iter) {
    // TODO: parse \u{xxx}
}

struct pattern_token pattern_token_wordlist(struct unicode_iter *iter) {
    // TODO: parse \w{name}
}

typedef struct pattern_token pattern_token_special(struct unicode_iter *iter);

struct special_chars {
  int32_t chr;
  int32_t dest;
  pattern_token_special *parser;
};

static struct special_chars special_chars[] = {
  {'a', '\a', NULL}, /// alert
  {'b', '\b', NULL}, /// no idea
  {'e', '\033', NULL}, /// escape
  {'f', '\f', NULL}, /// no idea
  {'n', '\n', NULL}, /// newline
  {'r', '\r', NULL}, /// carriage return
  {'t', '\t', NULL}, /// tab
  {'v', '\v', NULL}, /// vertical tab
  {'\\', '\\', NULL}, /// backspace
  {'u', 0, pattern_token_unicode},
  {'w', 0, pattern_token_wordlist},
};

static const size_t special_chars_size = sizeof(special_chars);

struct pattern_token pattern_token_next_complex(struct unicode_iter *iter) {
  struct unicode_iter_result result;

  // this is not a regular token. see what comes next.
  result = unicode_iter_next(iter);
  if(!result.ok) {
    return pattern_token_error(iter->pos);
  }

  for(size_t i = 0; i < special_chars_size; ++i) {
    if(result.codepoint == special_chars[i].chr) {
      if(special_chars[i].parser) {
        return special_chars[i].parser(iter);
      } else {
        return pattern_token_escaped(special_chars[i].dest);
      }
    }
  }

  return pattern_token_error(iter->pos);
}

/// Parse the next token, advancing the unicode reader.
struct pattern_token pattern_token_next(struct unicode_iter *iter) {
  struct unicode_iter_result result;

  result = unicode_iter_next(iter);
  if(!result.ok) {
    if(result.error == 0 && result.codepoint == 0) {
      return pattern_token_eof();
    } else {
      return pattern_token_error(iter->pos);
    }
  }

  // this is a regular token.
  if(result.codepoint != '\\') {
    return pattern_token_regular(result.codepoint);
  }

  return pattern_token_next_complex(iter);
}
