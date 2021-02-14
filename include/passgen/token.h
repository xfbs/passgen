/// @file token.h
/// @author Patrick M. Elsen
/// @brief Token Parsing functions.
///
/// Tokens are typically individual symbols, but can also be
/// escaped symbols, or special tokens.
#pragma once
#include <stddef.h>
#include <stdint.h>

struct passgen_token;
struct passgen_token_parser;

/// parse a single codepoint. The return value signals what happened. If it
/// returns zero (PASSGEN_TOKEN_INIT), it means that a token has been parsed into
/// `token`. If it returns a positive integer, it means that it was successful
/// but the token hasn't finished parsing yet, it is awaiting more input. If it
/// returns negatively, it means that there has been an error.
int passgen_token_parse(
    struct passgen_token_parser *parser,
    struct passgen_token *token,
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
