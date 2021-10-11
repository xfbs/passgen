#pragma once
#include <stdint.h>

#include "passgen/data/pattern.h"
#include "passgen/data/range.h"
#include "passgen/data/segment.h"
#include "passgen/data/segment_item.h"
#include "passgen/token.h"

/// Represents a parser. Contains a parsing stack, as well as a pattern, which
/// is dynamically built, and should not be used until the parsing is finalized.
struct passgen_parser {
    passgen_stack state;
    passgen_pattern_t pattern;
};

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
