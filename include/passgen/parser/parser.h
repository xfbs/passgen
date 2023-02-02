/// @file parser.h
/// @author Patrick M. Elsen
/// @brief Passgen syntax parser.
///
/// Parsing API. Needs a `passgen_parser` struct instance, which is defined in @ref data/parser.h.
///
/// # Example
///
/// ```c
/// #include <passgen/pattern/parser.h>
/// #include <passgen/parser/parser.h>
///
/// struct passgen_parser parser = {0};
/// passgen_parser_init(&parser);
/// passgen_parser_utf8(&parser, "[a-z]{9}", 0);
/// assert(0 == passgen_parser_finish(&parser));
/// // ...
/// passgen_parser_free(&parser);
/// ```
#pragma once
#include <stdint.h>
#include <stddef.h>

#include "passgen/pattern/pattern.h"
#include "passgen/pattern/range.h"
#include "passgen/pattern/segment.h"
#include "passgen/pattern/segment_item.h"
#include "passgen/parser/token.h"

typedef struct passgen_parser_state passgen_parser_state;

/// Represents a parser. Contains a parsing stack, as well as a pattern, which
/// is dynamically built, and should not be used until the parsing is finalized.
typedef struct passgen_parser {
    /// Current state of the parser, as a stack.
    passgen_stack state;
    /// Limit of parsing depth
    size_t limit;
    /// The current, possibly incomplete parsed pattern.
    passgen_pattern *pattern;
} passgen_parser;

/// Get the nth memeber of the parsing stack.
passgen_parser_state *
passgen_parser_get_state(passgen_parser *parser, size_t n);

/// Get the last item in the parsing stack.
passgen_parser_state *
passgen_parser_get_state_last(passgen_parser *parser);

/// Push a state item onto the stack and return a pointer to it.
passgen_parser_state *
passgen_parser_state_push(passgen_parser *parser);

/// Push a group item onto the parsing stack.
passgen_parser_state *passgen_parser_state_push_group(
    passgen_parser *parser,
    passgen_pattern_group *group,
    passgen_pattern_segment *segment);

/// Push a multiplier item onto the parsing stack.
passgen_parser_state *passgen_parser_state_push_multiplier(
    passgen_parser *parser,
    size_t *multiplier);

/// Push a set item onto the parsing stack.
passgen_parser_state *passgen_parser_state_push_set(
    passgen_parser *parser,
    passgen_pattern_set *set,
    passgen_pattern_range *range);

/// Push a repeat item onto the parsing stack.
passgen_parser_state *passgen_parser_state_push_repeat(
    passgen_parser *parser,
    passgen_pattern_repeat *repeat);

/// Push a special item into the parsing stack.
passgen_parser_state *passgen_parser_state_push_special(
    passgen_parser *parser,
    passgen_pattern_special *special);

/// Initialize the parsing structure.
///
/// Must be called once before the parser can be used. If @ref pattern is not
/// supplied, a new pattern will be allocated. This needs to be deallocated
/// (using @ref passgen_pattern_free and free).
void passgen_parser_init(passgen_parser *parser, passgen_pattern *pattern);

/// Set parsing depth limit for this parser.
///
/// This will prevent the parser from parsing recursive structures more than
/// @ref limit deep. Setting it to zero disables it.
void passgen_parser_limit(passgen_parser *parser, size_t limit);

/// Release memory used by the parser.
///
/// @return The parsed pattern
passgen_pattern *passgen_parser_free(passgen_parser *parser);

/// Finalize parsing.
///
/// @return 0 on success, nonzero on failure.
int passgen_parse_finish(struct passgen_parser *parser);

/// Parse a UTF-8 string.
int passgen_parser_utf8(
    struct passgen_parser *parser,
    uint8_t *data,
    size_t length);

/// Parse a sequence of unicode codepoints.
int passgen_parser_unicode(
    struct passgen_parser *parser,
    uint32_t *data,
    size_t length);

/// Parse a single token.
int passgen_parse_token(
    struct passgen_parser *parser,
    struct passgen_token *token);

int passgen_parse_group(
    struct passgen_parser *parser,
    struct passgen_token *token,
    struct passgen_parser_state *state);

int passgen_parse_multiplier(
    struct passgen_parser *parser,
    struct passgen_token *token,
    struct passgen_parser_state *state);

int passgen_parse_set(
    struct passgen_parser *parser,
    struct passgen_token *token,
    struct passgen_parser_state *state);

int passgen_parse_set_range(
    struct passgen_parser *parser,
    struct passgen_token *token,
    struct passgen_parser_state *state);

int passgen_parse_repeat(
    struct passgen_parser *parser,
    struct passgen_token *token,
    struct passgen_parser_state *state);

int passgen_parse_repeat_range(
    struct passgen_parser *parser,
    struct passgen_token *token,
    struct passgen_parser_state *state);

int passgen_parse_special(
    struct passgen_parser *parser,
    struct passgen_token *token,
    struct passgen_parser_state *state);

int passgen_parse_special_name(
    struct passgen_parser *parser,
    struct passgen_token *token,
    struct passgen_parser_state *state);
