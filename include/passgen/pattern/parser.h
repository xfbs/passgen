/// @file parser.h
/// @author Patrick M. Elsen <pelsen@xfbs.net>
/// @brief Passgen pattern parser data structures
#pragma once
#include <stdint.h>

#include "passgen/pattern/pattern.h"
#include "passgen/pattern/range.h"
#include "passgen/pattern/segment.h"
#include "passgen/pattern/segment_item.h"
#include "passgen/parser/token.h"

/// Represents a parser. Contains a parsing stack, as well as a pattern, which
/// is dynamically built, and should not be used until the parsing is finalized.
typedef struct passgen_parser {
    /// Current state of the parser, as a stack.
    passgen_stack state;
    /// The current, possibly incomplete parsed pattern.
    passgen_pattern pattern;
} passgen_parser;

/// Get the nth memeber of the parsing stack.
struct passgen_parser_state *
passgen_parser_get_state(struct passgen_parser *parser, size_t n);

/// Get the last item in the parsing stack.
struct passgen_parser_state *
passgen_parser_get_state_last(struct passgen_parser *parser);

/// Push a state item onto the stack and return a pointer to it.
struct passgen_parser_state *
passgen_parser_state_push(struct passgen_parser *parser);

/// Push a group item onto the parsing stack.
struct passgen_parser_state *passgen_parser_state_push_group(
    struct passgen_parser *parser,
    struct passgen_pattern_group *group,
    struct passgen_pattern_segment *segment);

/// Push a set item onto the parsing stack.
struct passgen_parser_state *passgen_parser_state_push_set(
    struct passgen_parser *parser,
    struct passgen_pattern_set *set,
    struct passgen_pattern_range *range);

/// Push a repeat item onto the parsing stack.
struct passgen_parser_state *passgen_parser_state_push_repeat(
    struct passgen_parser *parser,
    struct passgen_pattern_repeat *repeat);

/// Push a special item into the parsing stack.
struct passgen_parser_state *passgen_parser_state_push_special(
    struct passgen_parser *parser,
    struct passgen_pattern_special *special);

/// Initialize the parsing structure. Must be called once before the parser can
/// be used.
void passgen_parser_init(struct passgen_parser *parser);

/// Release memory used by the parser.
void passgen_parser_free(struct passgen_parser *parser);
