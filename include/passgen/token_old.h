#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "unicode.h"

/// Represents a valid substring of a string.
struct passgen_substring {
    size_t offset;
    size_t length;
};

enum passgen_token_type {
    PATTERN_TOKEN_EOF,
    PATTERN_TOKEN_REGULAR,
    PATTERN_TOKEN_SPECIAL,
    PATTERN_TOKEN_UNICODE,
    PATTERN_TOKEN_ESCAPED,
    PASSGEN_TOKEN_ERROR,
    PASSGEN_TOKEN_ERROR_ESCAPE,
    PASSGEN_TOKEN_ERROR_LBRACE,
    PASSGEN_TOKEN_ERROR_UNICODE_HEX,
    PASSGEN_TOKEN_ERROR_UNICODE_CHAR,
    PASSGEN_TOKEN_ERROR_UNICODE_EMPTY,
    PASSGEN_TOKEN_ERROR_UTF8 = (1 << 10),
};

struct passgen_token {
    enum passgen_token_type type;
    struct passgen_substring pos;
    struct passgen_substring data;
    size_t error;
    int32_t codepoint;
};

typedef struct passgen_token passgen_token_t;

/// Parse the next token, without advancing the unicode reader.
passgen_token_t passgen_token_peek(const unicode_iter_t *iter);

/// Parse the next token, advancing the unicode reader.
passgen_token_t passgen_token_next(unicode_iter_t *iter);

bool passgen_token_is_normal(passgen_token_t *token);
bool passgen_token_is_regular(passgen_token_t *token);
bool passgen_token_is_escaped(passgen_token_t *token);
bool passgen_token_is_unicode(passgen_token_t *token);
bool passgen_token_is_special(passgen_token_t *token);
bool passgen_token_is_eof(passgen_token_t *token);
bool passgen_token_is_error(passgen_token_t *token);
