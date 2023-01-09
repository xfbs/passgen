#include "passgen/parser/parser.h"

#include <stddef.h>

#include "passgen/pattern/parser.h"
#include "passgen/pattern/parser_state.h"

struct passgen_pattern_group;
struct passgen_pattern_item;
struct passgen_pattern_range;
struct passgen_pattern_segment;
struct passgen_pattern_set;

struct passgen_parser_state *
passgen_parser_state_push(struct passgen_parser *parser) {
    return passgen_stack_push(&parser->state, NULL);
}

struct passgen_parser_state *passgen_parser_state_push_group(
    struct passgen_parser *parser,
    struct passgen_pattern_group *group,
    struct passgen_pattern_segment *segment) {
    struct passgen_parser_state *state =
        passgen_stack_push(&parser->state, NULL);
    state->type = PASSGEN_PARSER_GROUP;
    state->data.group.group = group;
    state->data.group.segment = segment;

    return state;
}

struct passgen_parser_state *passgen_parser_state_push_set(
    struct passgen_parser *parser,
    struct passgen_pattern_set *set,
    struct passgen_pattern_range *range) {
    struct passgen_parser_state *state =
        passgen_stack_push(&parser->state, NULL);
    state->type = PASSGEN_PARSER_SET;
    state->data.set.set = set;
    state->data.set.range = range;

    return state;
}

struct passgen_parser_state *passgen_parser_state_push_repeat(
    struct passgen_parser *parser,
    struct passgen_pattern_repeat *repeat) {
    struct passgen_parser_state *state =
        passgen_stack_push(&parser->state, NULL);
    state->type = PASSGEN_PARSER_REPEAT;
    repeat->min = 0;
    repeat->max = 0;
    state->data.repeat.repeat = repeat;

    return state;
}

struct passgen_parser_state *passgen_parser_state_push_special(
    struct passgen_parser *parser,
    struct passgen_pattern_special *special) {
    struct passgen_parser_state *state =
        passgen_stack_push(&parser->state, NULL);
    state->type = PASSGEN_PARSER_SPECIAL;
    state->data.special.special = special;

    return state;
}

void passgen_parser_init(struct passgen_parser *parser) {
    passgen_stack_init(&parser->state, sizeof(struct passgen_parser_state));
    passgen_pattern_init(&parser->pattern);
    passgen_parser_state_push_group(
        parser,
        &parser->pattern.group,
        passgen_pattern_group_new_segment(&parser->pattern.group));
}

void passgen_parser_free(struct passgen_parser *parser) {
    passgen_stack_free(&parser->state);
    passgen_pattern_free(&parser->pattern);
}

struct passgen_parser_state *
passgen_parser_get_state(struct passgen_parser *parser, size_t n) {
    return passgen_stack_get(&parser->state, n);
}

struct passgen_parser_state *
passgen_parser_get_state_last(struct passgen_parser *parser) {
    return passgen_stack_top(&parser->state);
}
