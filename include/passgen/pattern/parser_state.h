#pragma once

#include "passgen/pattern/pattern.h"

enum passgen_parser_state_type {
    PASSGEN_PARSER_GROUP,
    PASSGEN_PARSER_SET,
    PASSGEN_PARSER_SET_RANGE,
    PASSGEN_PARSER_REPEAT,
    PASSGEN_PARSER_REPEAT_RANGE,
    PASSGEN_PARSER_SPECIAL,
    PASSGEN_PARSER_SPECIAL_NAME,
    PASSGEN_PARSER_DONE,
};

/// Represents the current state of the parser.
///
/// The parser is built as a state machine, with a state stack representing
/// what it is currently doing. This allows for introspection of the parser,
/// limiting the depth (by limiting the stack size) and for giving useful error
/// messages.
struct passgen_parser_state {
    enum passgen_parser_state_type type;
    union {
        struct {
            struct passgen_pattern_group *group;
            struct passgen_pattern_segment *segment;
        } group;
        struct {
            struct passgen_pattern_repeat *repeat;
        } repeat;
        struct {
            struct passgen_pattern_set *set;
            struct passgen_pattern_range *range;
        } set;
        struct {
            struct passgen_pattern_special *special;
        } special;
    } data;
};

void passgen_parser_state_free(struct passgen_parser_state *state);
