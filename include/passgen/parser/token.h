/// @file token.h
/// @author Patrick M. Elsen
/// @brief Functions to parse Unicode codepoints into @ref passgen_token values.
///
/// The input to Passgen at this point is a sequence of unicode codepoints. These are
/// converted into tokens. For most codepoints, every codepoint maps one-to-one to a token,
/// but there are some special cases such as unicode escape sequences (for example `\u{0a}`
/// will get you a space).
///
/// Tokens also handle escaping characters. For example, to write a closing bracket in a
/// character range, you can escape it with a backslash (such as `[a-z&=\]]`).
#pragma once
#include <stddef.h>
#include <stdint.h>

struct passgen_token;
struct passgen_token_parser;

/// Parse a single codepoint.
///
/// This function parses a single codepoint into a `passgen_token`. It needs an initialized
/// parser to do so, and will return a value indicating if the parsing was successful or not.
/// It can also be passed a width, which indicates the byte width of the codepoint (if it was
/// decoded from a UTF-8 string) which is used to keep track of the exact byte offset for
/// every token to make diagnostic error reporting easier.
///
/// @param parser An initialized `passgen_token_parser`. Use `passgen_token_parser_init` to
///   initialize it if you are not sure.
/// @param token The token that is parsed, this is an output.
/// @param width The width (in bytes) of the codepoint. This can be set to 1 if the original
///   input was Unicode codepoints.
/// @param codepoint The codepoint to parse.
/// @return The status of the parsing. This is an @ref passgen_token_state enum value.
int passgen_token_parse(
    struct passgen_token_parser *parser,
    struct passgen_token *token,
    uint8_t width,
    uint32_t codepoint);

/// Parse a bunch of codepoints from an array. The variable size should contain
/// the number of codepoints and the number of slots in the token array (should
/// be the same).
///
/// It reads `size` amount of codepoints from the codepoints array, and writes
/// up to `size` amount of tokens into the tokens array.
///
/// If the return value is zero or positive, it was a success, and `size` is
/// filled with the amount of tokens in the tokens array.
int passgen_token_parse_str(
    struct passgen_token_parser *parser,
    size_t *size,
    struct passgen_token *token,
    uint32_t *codepoints);

const char *passgen_token_parse_error_str(int ret);
