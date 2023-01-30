#include "passgen/parser/parser.h"

#include <stddef.h>
#include <stdlib.h>

#include "passgen/pattern/parser.h"
#include "passgen/pattern/parser_state.h"

passgen_parser_state *passgen_parser_state_push(passgen_parser *parser) {
    return passgen_stack_push(&parser->state, NULL);
}

passgen_parser_state *passgen_parser_state_push_group(
    passgen_parser *parser,
    passgen_pattern_group *group,
    passgen_pattern_segment *segment) {
    passgen_parser_state *state = passgen_stack_push(&parser->state, NULL);
    state->type = PASSGEN_PARSER_GROUP;
    state->data.group.group = group;
    state->data.group.segment = segment;

    return state;
}

passgen_parser_state *passgen_parser_state_push_set(
    passgen_parser *parser,
    passgen_pattern_set *set,
    passgen_pattern_range *range) {
    passgen_parser_state *state = passgen_stack_push(&parser->state, NULL);
    state->type = PASSGEN_PARSER_SET;
    state->data.set.set = set;
    state->data.set.range = range;

    return state;
}

passgen_parser_state *passgen_parser_state_push_repeat(
    passgen_parser *parser,
    passgen_pattern_repeat *repeat) {
    passgen_parser_state *state = passgen_stack_push(&parser->state, NULL);
    state->type = PASSGEN_PARSER_REPEAT;
    repeat->min = 0;
    repeat->max = 0;
    state->data.repeat.repeat = repeat;

    return state;
}

passgen_parser_state *passgen_parser_state_push_multiplier(
    passgen_parser *parser,
    size_t *multiplier) {
    passgen_parser_state *state = passgen_stack_push(&parser->state, NULL);
    state->type = PASSGEN_PARSER_MULTIPLIER;
    *multiplier = 0;
    state->data.multiplier = multiplier;
    return state;
}

passgen_parser_state *passgen_parser_state_push_special(
    passgen_parser *parser,
    passgen_pattern_special *special) {
    passgen_parser_state *state = passgen_stack_push(&parser->state, NULL);
    state->type = PASSGEN_PARSER_SPECIAL;
    state->data.special.special = special;
    return state;
}

void passgen_parser_init(passgen_parser *parser, passgen_pattern *pattern) {
    passgen_stack_init(&parser->state, sizeof(passgen_parser_state));
    parser->pattern = pattern;
    if(!pattern) {
        parser->pattern = malloc(sizeof(passgen_pattern));
    }
    passgen_pattern_init(parser->pattern);
    passgen_parser_state_push_group(
        parser,
        &parser->pattern->group,
        passgen_pattern_group_new_segment(&parser->pattern->group));
}

passgen_pattern *passgen_parser_free(passgen_parser *parser) {
    passgen_stack_free(&parser->state);
    passgen_pattern *pattern = parser->pattern;
    parser->pattern = NULL;
    return pattern;
}

passgen_parser_state *
passgen_parser_get_state(passgen_parser *parser, size_t n) {
    return passgen_stack_get(&parser->state, n);
}

passgen_parser_state *passgen_parser_get_state_last(passgen_parser *parser) {
    return passgen_stack_top(&parser->state);
}
