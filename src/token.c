#include "passgen/token.h"
#include <stdlib.h>
#include <utf8proc.h>

static inline passgen_token_t
passgen_token_error_utf8(size_t start, size_t pos, size_t end, ssize_t error) {
  return (passgen_token_t) {
    .type = PASSGEN_TOKEN_ERROR_UTF8 + llabs(error),
    .pos.offset = start,
    .pos.length = end - start,
    .error = pos,
  };
}

static inline passgen_token_t
passgen_token_error_escape(size_t start, size_t pos, size_t end) {
  return (passgen_token_t) {
    .type = PASSGEN_TOKEN_ERROR_ESCAPE,
    .pos.offset = start,
    .pos.length = end - start,
    .error = pos,
  };
}

static inline passgen_token_t
passgen_token_error_lbrace(size_t start, size_t pos, size_t end) {
  return (passgen_token_t) {
    .type = PASSGEN_TOKEN_ERROR_LBRACE,
    .pos.offset = start,
    .pos.length = end - start,
    .error = pos,
  };
}

static inline passgen_token_t
passgen_token_error_unicode_hex(
        size_t start,
        size_t data_start,
        size_t error,
        size_t end)
{
  return (passgen_token_t) {
    .type = PASSGEN_TOKEN_ERROR_UNICODE_HEX,
    .pos.offset = start,
    .pos.length = end - start,
    .data.offset = data_start,
    .data.length = end - data_start,
    .error = error,
  };
}

static inline passgen_token_t
passgen_token_error_unicode_empty(
        size_t start,
        size_t end,
        size_t data_start)
{
  return (passgen_token_t) {
    .type = PASSGEN_TOKEN_ERROR_UNICODE_EMPTY,
    .pos.offset = start,
    .pos.length = end - start,
    .data.offset = data_start,
    .data.length = 0,
    .error = data_start,
  };
}

static inline passgen_token_t
passgen_token_error_unicode_char(
        size_t start,
        size_t end,
        size_t data_start,
        size_t data_end)
{
  return (passgen_token_t) {
    .type = PASSGEN_TOKEN_ERROR_UNICODE_CHAR,
    .pos.offset = start,
    .pos.length = end - start,
    .data.offset = data_start,
    .data.length = data_end - data_start,
    .error = data_start,
  };
}

static inline passgen_token_t
passgen_token_eof(size_t start) {
  return (passgen_token_t) {
    .type = PATTERN_TOKEN_EOF,
    .pos.offset = start,
    .pos.length = 0,
  };
}

static inline passgen_token_t
passgen_token_regular(size_t start, size_t end, int32_t codepoint) {
  return (passgen_token_t) {
    .type = PATTERN_TOKEN_REGULAR,
    .codepoint = codepoint,
    .pos.offset = start,
    .pos.length = end - start,
  };
}

static inline passgen_token_t
passgen_token_escaped(
        size_t start,
        size_t end,
        int32_t codepoint) {
  return (passgen_token_t) {
    .type = PATTERN_TOKEN_ESCAPED,
    .codepoint = codepoint,
    .pos.offset = start,
    .pos.length = end - start,
  };
}

static inline passgen_token_t
passgen_token_simple(
        size_t start,
        size_t end,
        int32_t codepoint,
        enum passgen_token_type type) {
  return (passgen_token_t) {
    .type = type,
    .codepoint = codepoint,
    .pos.offset = start,
    .pos.length = end - start,
  };
}

static inline passgen_token_t
passgen_token_unicode(
        size_t start,
        size_t end,
        size_t data_start,
        size_t data_end,
        int32_t codepoint)
{
  return (passgen_token_t) {
    .type = PATTERN_TOKEN_UNICODE,
    .codepoint = codepoint,
    .pos.offset = start,
    .pos.length = end - start,
    .data.offset = data_start,
    .data.length = data_end - data_start,
  };
}

/// Parse the next token, without advancing the unicode reader.
passgen_token_t passgen_token_peek(const struct unicode_iter *iter) {
  struct unicode_iter iter_copy = *iter;
  return passgen_token_next(&iter_copy);
}

passgen_token_t passgen_token_parse_unicode(size_t start, unicode_iter_t *iter) {
    size_t pos = iter->pos;
    unicode_iter_result_t result;
    int32_t codepoint = 0;

    result = unicode_iter_next(iter);
    if(!result.ok) {
        return passgen_token_error_utf8(start, pos, iter->pos, result.error);
    }

    if(result.codepoint != '{') {
        return passgen_token_error_lbrace(start, pos, iter->pos);
    }

    size_t data_start = iter->pos;
    size_t data_len_codepoints = 0;
    pos = iter->pos;
    result = unicode_iter_next(iter);
    while(result.ok && result.codepoint != '}') {
        data_len_codepoints += 1;
        codepoint *= 16;

        if(6 < data_len_codepoints) {
            // hex is too long. unicode is valid from 0 to 10FFFF, afaik.
            return passgen_token_error_unicode_char(start, iter->pos, data_start, pos);
        }

        if('0' <= result.codepoint && result.codepoint <= '9') {
            codepoint += result.codepoint - '0';
        } else if('a' <= result.codepoint && result.codepoint <= 'f') {
            codepoint += result.codepoint - 'a' + 10;
        } else if('A' <= result.codepoint && result.codepoint <= 'F') {
            codepoint += result.codepoint - 'A' + 10;
        } else {
            return passgen_token_error_unicode_hex(start, data_start, pos, iter->pos);
        }

        pos = iter->pos;
        result = unicode_iter_next(iter);
    }

    // unicode error.
    if(!result.ok) {
        return passgen_token_error_utf8(start, pos, iter->pos, result.error);
    }

    if(data_len_codepoints == 0) {
        return passgen_token_error_unicode_empty(start, iter->pos, data_start);
    }

    if(utf8proc_codepoint_valid(codepoint) != 1) {
        return passgen_token_error_unicode_char(start, iter->pos, data_start, pos);
    }

    return passgen_token_unicode(start, iter->pos, data_start, pos, codepoint);
}

passgen_token_t passgen_token_wordlist(size_t start, unicode_iter_t *iter) {
    // TODO: parse \w{name}
}

typedef passgen_token_t passgen_token_special(size_t start, unicode_iter_t *iter);

struct special_chars {
    int32_t chr;
    int32_t codepoint;
    enum passgen_token_type type;
    passgen_token_special *parser;
};

const static struct special_chars special_chars[] = {
    {'a', '\a', PATTERN_TOKEN_ESCAPED, NULL},
    {'b', '\b', PATTERN_TOKEN_ESCAPED, NULL},
    {'e', '\033', PATTERN_TOKEN_ESCAPED, NULL},
    {'f', '\f', PATTERN_TOKEN_ESCAPED, NULL},
    {'n', '\n', PATTERN_TOKEN_ESCAPED, NULL},
    {'r', '\r', PATTERN_TOKEN_ESCAPED, NULL},
    {'t', '\t', PATTERN_TOKEN_ESCAPED, NULL},
    {'v', '\v', PATTERN_TOKEN_ESCAPED, NULL},
    {'\\', '\\', PATTERN_TOKEN_ESCAPED, NULL},
    {'w', 'w', PATTERN_TOKEN_SPECIAL, NULL},
    {'p', 'p', PATTERN_TOKEN_SPECIAL, NULL},
    {'u', 0, 0, passgen_token_parse_unicode},
    {0, 0, 0, NULL},
};

static const size_t special_chars_size = 12;

static const char escapable_chars[] = {'(', ')', '[', ']', '{', '}', '|'};
static const size_t escapable_chars_len = sizeof(escapable_chars);

passgen_token_t passgen_token_next_complex(size_t start, unicode_iter_t *iter) {
    size_t pos = iter->pos;
    unicode_iter_result_t result;

    // this is not a regular token. see what comes next.
    result = unicode_iter_next(iter);
    if(!result.ok) {
        return passgen_token_error_utf8(start, pos, iter->pos, result.error);
    }

    // check for regular escaped chars.
    for(size_t i = 0; i < escapable_chars_len; i++) {
        if(result.codepoint == escapable_chars[i]) {
            return passgen_token_escaped(start, iter->pos, escapable_chars[i]);
        }
    }

    for(size_t i = 0; i < special_chars_size; ++i) {
        if(result.codepoint == special_chars[i].chr) {
            if(special_chars[i].parser) {
                return special_chars[i].parser(start, iter);
            } else {
                return passgen_token_simple(
                        start,
                        iter->pos,
                        special_chars[i].codepoint,
                        special_chars[i].type);
            }
        }
    }

    return passgen_token_error_escape(start, pos, iter->pos);
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
        return passgen_token_regular(start, iter->pos, result.codepoint);
    }

    return passgen_token_next_complex(start, iter);
}

bool passgen_token_is_normal(passgen_token_t *token) {
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
    return token->type == PATTERN_TOKEN_EOF;
}

bool passgen_token_is_error(passgen_token_t *token) {
    return token->type >= PASSGEN_TOKEN_ERROR;
}

bool passgen_token_is_unicode(passgen_token_t *token) {
    return token->type == PATTERN_TOKEN_UNICODE;
}

bool passgen_token_is_regular(passgen_token_t *token) {
    return token->type == PATTERN_TOKEN_REGULAR;
}

bool passgen_token_is_special(passgen_token_t *token) {
    return token->type == PATTERN_TOKEN_SPECIAL;
}

bool passgen_token_is_escaped(passgen_token_t *token) {
    return token->type == PATTERN_TOKEN_ESCAPED;
}
