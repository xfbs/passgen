/// @file state.h
/// @author Patrick M. Elsen
/// @brief Parser state item.
#pragma once

#include "passgen/pattern/pattern.h"
#include "passgen/pattern/group.h"
#include "passgen/pattern/segment.h"
#include "passgen/pattern/repeat.h"
#include "passgen/pattern/set.h"
#include "passgen/pattern/range.h"

typedef enum passgen_parser_state_type {
    PASSGEN_PARSER_GROUP,
    PASSGEN_PARSER_MULTIPLIER,
    PASSGEN_PARSER_SET,
    PASSGEN_PARSER_SET_RANGE,
    PASSGEN_PARSER_REPEAT,
    PASSGEN_PARSER_REPEAT_RANGE,
    PASSGEN_PARSER_SPECIAL,
    PASSGEN_PARSER_SPECIAL_NAME,
    PASSGEN_PARSER_DONE,
} passgen_parser_state_type;

/// Represents the current state of the parser.
///
/// The parser is built as a state machine, with a state stack representing
/// what it is currently doing. This allows for introspection of the parser,
/// limiting the depth (by limiting the stack size) and for giving useful error
/// messages.
typedef struct passgen_parser_state {
    enum passgen_parser_state_type type;
    union {
        struct {
            passgen_pattern_group *group;
            passgen_pattern_segment *segment;
        } group;
        struct {
            passgen_pattern_repeat *repeat;
        } repeat;
        struct {
            passgen_pattern_set *set;
            passgen_pattern_range *range;
        } set;
        struct {
            passgen_pattern_special *special;
        } special;
        size_t *multiplier;
    } data;
} passgen_parser_state;

void passgen_parser_state_free(passgen_parser_state *state);
