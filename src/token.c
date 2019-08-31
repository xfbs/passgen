#include "passgen/token.h"
#include <stdlib.h>

passgen_token_t passgen_token_error_utf8(size_t start, size_t pos, size_t end, ssize_t error) {
  return (passgen_token_t) {
    .type = PASSGEN_TOKEN_ERROR_UTF8 + llabs(error),
    .pos.offset = start,
    .pos.length = end - start,
    .data.offset = pos,
    .data.length = end - pos,
  };
}

passgen_token_t passgen_token_error_escape_illegal(size_t start, size_t pos, size_t end) {
  return (passgen_token_t) {
    .type = PASSGEN_TOKEN_ERROR_ESCAPE_ILLEGAL,
    .pos.offset = start,
    .pos.length = end - start,
    .data.offset = pos,
    .data.length = end - pos,
  };
}

passgen_token_t passgen_token_eof(size_t start) {
  return (passgen_token_t) {
    .type = PATTERN_TOKEN_EOF,
    .pos.offset = start,
    .pos.length = 0,
  };
}

passgen_token_t passgen_token_regular(size_t start, size_t len, int32_t codepoint) {
  return (passgen_token_t) {
    .type = PATTERN_TOKEN_REGULAR,
    .codepoint = codepoint
  };
}

passgen_token_t passgen_token_escaped(size_t start, size_t end, int32_t codepoint) {
  return (passgen_token_t) {
    .type = PATTERN_TOKEN_ESCAPED,
    .codepoint = codepoint,
    .pos.offset = start,
    .pos.length = end - start,
  };
}

/// Parse the next token, without advancing the unicode reader.
passgen_token_t passgen_token_peek(const struct unicode_iter *iter) {
  struct unicode_iter iter_copy = *iter;
  return passgen_token_next(&iter_copy);
}

passgen_token_t passgen_token_unicode(struct unicode_iter *iter) {
    // TODO: parse \u{xxx}
}

passgen_token_t passgen_token_wordlist(struct unicode_iter *iter) {
    // TODO: parse \w{name}
}

typedef passgen_token_t passgen_token_special(struct unicode_iter *iter);

struct special_chars {
  int32_t chr;
  int32_t dest;
  passgen_token_special *parser;
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
  {'u', 0, passgen_token_unicode},
  {'w', 0, passgen_token_wordlist},
};

static const size_t special_chars_size = sizeof(special_chars);

passgen_token_t passgen_token_next_complex(size_t start, unicode_iter_t *iter) {
    size_t pos = iter->pos;
    unicode_iter_result_t result;

    // this is not a regular token. see what comes next.
    result = unicode_iter_next(iter);
    if(!result.ok) {
        return passgen_token_error_utf8(start, pos, iter->pos, result.error);
    }

    for(size_t i = 0; i < special_chars_size; ++i) {
        if(result.codepoint == special_chars[i].chr) {
            if(special_chars[i].parser) {
                return special_chars[i].parser(iter);
            } else {
                return passgen_token_escaped(start, iter->pos, special_chars[i].dest);
            }
        }
    }

    return passgen_token_error_escape_illegal(start, pos, iter->pos);
}

/// Parse the next token, advancing the unicode reader.
passgen_token_t passgen_token_next(unicode_iter_t *iter) {
    size_t start = iter->pos;
    unicode_iter_result_t result;

    result = unicode_iter_next(iter);
    if(!result.ok) {
        if(result.error == 0 && result.codepoint == 0) {
            return passgen_token_eof(start);
        } else {
            return passgen_token_error_utf8(start, start, iter->pos, result.error);
        }
    }

    // this is a regular token.
    if(result.codepoint != '\\') {
        return passgen_token_regular(start, iter->pos - start, result.codepoint);
    }

    return passgen_token_next_complex(start, iter);
}

bool passgen_token_is_normal(passgen_token_t *token) {
    if(!token->ok) {
        return false;
    }

    switch(token->type) {
        case PATTERN_TOKEN_REGULAR:
        case PATTERN_TOKEN_ESCAPED:
        case PATTERN_TOKEN_UNICODE:
            return true;
        default:
            return false;
    }
}

bool passgen_token_is_eof(passgen_token_t *token) {
    return token->ok && token->type == PATTERN_TOKEN_EOF;
}

bool passgen_token_is_result(passgen_token_t *token) {
}
