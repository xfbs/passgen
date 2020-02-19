/* Token Parsing functions.
 *
 * Tokens are typically individual symbols, but can also be
 * escaped symbols, or special tokens.
 */

#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// when updating any of the enums, make sure to also update the mappings.
enum token_state {
    TOKEN_INIT = 0,
    TOKEN_ESCAPED,
    TOKEN_UNICODE,
    TOKEN_UNICODE_PAYLOAD,

    // errors
    TOKEN_ERROR_UNICODE_START = -1,
    TOKEN_ERROR_UNICODE_PAYLOAD = -2,
    TOKEN_ERROR_UNICODE_PAYLOAD_LEN = -3
};

enum token_type {
    TOKEN_NORMAL,
    TOKEN_SPECIAL,
    TOKEN_ESCAPED_TYPE,
    TOKEN_ESCAPED_SIMPLE_TYPE,
    TOKEN_ESCAPED_PASSTHRU,
    TOKEN_ESCAPED_UNICODE
};

enum token_escaped {
    TOKEN_ESCAPED_NOT,
    TOKEN_ESCAPED_SIMPLE,
    TOKEN_ESCPAED_NORMAL
};

struct token_parser {
    enum token_state state;
    union {
        // when parsing a \u{FA} unicode literal, this is where we keep the
        // state.
        struct {
            size_t length;
            uint32_t codepoint;
        } unicode_payload;
    } data;
};

struct token {
    uint32_t codepoint;
    enum token_type type;
    bool escaped;
    bool normal_escaped;
};

struct enum_mapping {
    int value;
    const char *name;
};

extern const struct enum_mapping token_state_mapping[];
extern const struct enum_mapping token_type_mapping[];
extern const struct enum_mapping token_escaped_mapping[];

/* parse a single codepoint. The return value signals what happened. If it
 * returns zero (TOKEN_INIT), it means that a token has been parsed into `token`.
 * If it returns a positive integer, it means that it was successful but the
 * token hasn't finished parsing yet, it is awaiting more input. If it returns
 * negatively, it means that there has been an error.
 */
int token_parse(struct token_parser *parser, struct token *token, uint32_t codepoint);

/* Parse a bunch of codepoints from an array. The variable size should contain
 * the number of codepoints and the number of slots in the token array (should be
 * the same). 
 *
 * It reads `size` amount of codepoints from the codepoints array, and writes
 * up to `size` amount of tokens into the tokens array. 
 *
 * If the return value is zero or positive, it was a success, and `size` is filled
 * with the amount of tokens in the tokens array.
 */
int token_parse_str(struct token_parser *parser, size_t *size, struct token token[], uint32_t codepoints[]);

const char *token_parse_error_str(int ret);

