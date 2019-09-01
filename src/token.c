#include "passgen/token.h"
#include <stdlib.h>

passgen_token_t passgen_token_error_utf8(size_t start, size_t pos, size_t end, ssize_t error) {
  return (passgen_token_t) {
    .type = PASSGEN_TOKEN_ERROR_UTF8 + llabs(error),
    .pos.offset = start,
    .pos.length = end - start,
    .error = pos,
  };
}

passgen_token_t passgen_token_error_escape(size_t start, size_t pos, size_t end) {
  return (passgen_token_t) {
    .type = PASSGEN_TOKEN_ERROR_ESCAPE,
    .pos.offset = start,
    .pos.length = end - start,
    .error = pos,
  };
}

passgen_token_t passgen_token_error_lbrace(size_t start, size_t pos, size_t end) {
  return (passgen_token_t) {
    .type = PASSGEN_TOKEN_ERROR_LBRACE,
    .pos.offset = start,
    .pos.length = end - start,
    .error = pos,
  };
}

passgen_token_t passgen_token_error_unicode_hex(
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

passgen_token_t passgen_token_error_unicode_char(
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

passgen_token_t passgen_token_eof(size_t start) {
  return (passgen_token_t) {
    .type = PATTERN_TOKEN_EOF,
    .pos.offset = start,
    .pos.length = 0,
  };
}

passgen_token_t passgen_token_regular(size_t start, size_t end, int32_t codepoint) {
  return (passgen_token_t) {
    .type = PATTERN_TOKEN_REGULAR,
    .codepoint = codepoint,
    .pos.offset = start,
    .pos.length = end - start,
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

passgen_token_t passgen_token_unicode(
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
    size_t data_end = 0;
    pos = iter->pos;
    result = unicode_iter_next(iter);
    while(result.ok && result.codepoint != '}') {
        data_len_codepoints += 1;
        codepoint *= 16;

        if(6 < data_len_codepoints) {
            // hex is too long. unicode is valid from 0 to 10FFFF, afaik.
            return passgen_token_error_unicode_char(start, iter->pos, data_start, data_end);
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

        data_end = pos;
        pos = iter->pos;
        result = unicode_iter_next(iter);
    }

    // unicode error.
    if(!result.ok) {
        return passgen_token_error_utf8(start, pos, iter->pos, result.error);
    }

    if(utf8proc_codepoint_valid(codepoint) != 1) {
        return passgen_token_error_unicode_char(start, iter->pos, data_start, data_end);
    }

    return passgen_token_unicode(start, iter->pos, data_start, data_end, codepoint);
}

passgen_token_t passgen_token_wordlist(size_t start, unicode_iter_t *iter) {
    // TODO: parse \w{name}
}

typedef passgen_token_t passgen_token_special(size_t start, unicode_iter_t *iter);

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
  {'u', 0, passgen_token_parse_unicode},
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
                return special_chars[i].parser(start, iter);
            } else {
                return passgen_token_escaped(start, iter->pos, special_chars[i].dest);
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

bool passgen_token_is_escaped(passgen_token_t *token) {
    return token->type == PATTERN_TOKEN_ESCAPED;
}
