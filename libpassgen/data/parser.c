#include "passgen/parser.h"

#include <stddef.h>

#include "passgen/data/parser.h"
#include "passgen/data/parser_state.h"
#include "passgen/container/stack/parser_state.h"
#include "passgen/data/pattern.h"

struct passgen_pattern_group;
struct passgen_pattern_item;
struct passgen_pattern_range;
struct passgen_pattern_segment;
struct passgen_pattern_set;

struct passgen_parser_state *parser_state_push(struct parser *parser) {
  return passgen_parser_state_stack_push(&parser->state, NULL);
}

struct passgen_parser_state *parser_state_push_group(
    struct parser *parser,
    struct passgen_pattern_group *group,
    struct passgen_pattern_segment *segment) {
  struct passgen_parser_state *state = parser_state_push(parser);
  state->type = PASSGEN_PARSER_GROUP;
  state->data.group.group = group;
  state->data.group.segment = segment;

  return state;
}

struct passgen_parser_state *parser_state_push_set(
    struct parser *parser,
    struct passgen_pattern_set *set,
    struct passgen_pattern_range *range) {
  struct passgen_parser_state *state = parser_state_push(parser);
  state->type = PASSGEN_PARSER_SET;
  state->data.set.set = set;
  state->data.set.range = range;

  return state;
}

struct passgen_parser_state *parser_state_push_repeat(
    struct parser *parser,
    struct passgen_pattern_item *item) {
  struct passgen_parser_state *state = parser_state_push(parser);
  state->type = PASSGEN_PARSER_REPEAT;
  state->data.repeat.item = item;

  return state;
}

void passgen_parser_init(struct parser *parser) {
  passgen_parser_state_stack_init(&parser->state, NULL);
  passgen_pattern_init(&parser->pattern);
}

void passgen_parser_free(struct parser *parser) {
  passgen_parser_state_stack_free(&parser->state, NULL);
  passgen_pattern_free(&parser->pattern);
}

void parser_state_pop(struct parser *parser) {
  passgen_parser_state_stack_pop(&parser->state, NULL);
}

struct passgen_parser_state *
passgen_parser_get_state(struct parser *parser, size_t n) {
  return passgen_parser_state_stack_get(&parser->state, n);
}

struct passgen_parser_state *
passgen_parser_get_state_last(struct parser *parser) {
  return passgen_parser_state_stack_top(&parser->state);
}
