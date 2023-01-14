/// @file token.h
/// @author Patrick M. Elsen <pelsen@xfbs.net>
/// @brief Token and token parser data structures and methods.
#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/// Bit that is ORed together with a parsed Unicode codepoint to indicate that
/// it was preceded by a backslash character.
#define PASSGEN_TOKEN_ESCAPED_BIT  (1 << 30)

/// Bitmask for extracting only the Unicode codepoint from the parsed @ref passgen_token.
#define PASSGEN_TOKEN_UNICODE_MASK (1 << 21 - 1)

/// State of the @ref passgen_token_parser.
///
/// Initial state is @ref PASSGEN_TOKEN_INIT, which has the integer value zero. The parser
/// returns to this state after successfully parsing a token.
/// Positive integer values mean that the parser is in the middle of parsing a multi-codepoint
/// token.
/// Negative values indicate errors, once the parser is in an error state it must be reset.
enum passgen_token_state {
    /// Initial state.
    PASSGEN_TOKEN_INIT = 0,
    /// Has parsed a backslash (`\`) and is currently parsing an escape sequence.
    PASSGEN_TOKEN_ESCAPED,
    /// Currently parsing a unicode escape sequence (`\u`).
    PASSGEN_TOKEN_UNICODE,
    /// Currently parsing the payload of a unicode escape sequence.
    PASSGEN_TOKEN_UNICODE_PAYLOAD,
    /// Error while parsing unicode escape sequence.
    PASSGEN_TOKEN_ERROR_UNICODE_START = -1,
    /// Error while parsing unicode escape sequence payload.
    PASSGEN_TOKEN_ERROR_UNICODE_PAYLOAD = -2,
    /// While parsing a unicode escape sequence, encountered a payload that was too large.
    PASSGEN_TOKEN_ERROR_UNICODE_PAYLOAD_LEN = -3
};

/// Parser state for tokenizer.
///
/// The parser keeps track of it's current state (@ref state). It keeps track of how many
/// codepoints (@ref offset) and bytes (@ref byte_offset) have been parsed so far, and writes
/// this information into the parsed tokens. This makes is possible later to generate useful error
/// messages referencing the exact location in the input that caused the error.
/// The parser also keeps some state (@ref unicode_payload) for parsing unicode literals. These allow
/// writing Unicode characters in ASCII by specifying them in hexadecimal notation. For example, a space
/// could be written as `\x{0a}`. To make this possible, the parser needs to keep track of the length of
/// the payload (the hexadecimal data), since that has an upper bound, and the current parsed codepoint
/// value.
typedef struct passgen_token_parser {
    /// Current state of the parser.
    enum passgen_token_state state;
    /// Current codepoint offset in parsing.
    size_t offset;
    /// Current byte offset.
    size_t byte_offset;
    union {
        struct {
            /// Length of the payload parsed so far in Unicode escape sequence.
            size_t length;
            /// Current parsed codepoint.
            uint32_t codepoint;
        } unicode_payload;
    } data;
} passgen_token_parser;

/// Parsed passgen token.
///
/// Contains the parsed Unicode @ref codepoint. If it was preceded by a backslash character,
/// it will be binary ORed with @ref PASSGEN_TOKEN_ESCAPED_BIT. In that case, to extract only
/// the codepoint, you can mask it with @ref PASSGEN_TOKEN_UNICODE_MASK:
/// `token.codepoint & PASSGEN_TOKEN_UNICODE_MASK`.
///
/// Also contains the byte offset of this token in the input string (useful if the input is
/// UTF-8 encoded) and the codepoint offset of this token.
typedef struct passgen_token {
    uint32_t codepoint;
    size_t offset;
    size_t byte_offset;
} passgen_token;

/// Initialize a @ref passgen_token_parser.
void passgen_token_parser_init(struct passgen_token_parser *token_parser);

/// Convert a @ref passgen_token_state into a string.
const char *passgen_token_state_string(enum passgen_token_state state);
