/* Token Parsing functions.
 *
 * Tokens are typically individual symbols, but can also be
 * escaped symbols, or special tokens.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

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
    TOKEN_SPECIAL
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

/* parse a single codepoint. The return value signals what happened. If it
 * returns zero (TOKEN_INIT), it means that a token has been parsed into `token`.
 * If it returns a positive integer, it means that it was successful but the
 * token hasn't finished parsing yet, it is awaiting more input. If it returns
 * negatively, it means that there has been an error.
 */
int token_parse(struct token_parser *parser, struct token *token, uint32_t codepoint);

const char *token_parse_error_str(int ret);
