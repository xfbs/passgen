#include "passgen/parser.h"
#include "passgen/pattern/item.h"
#include "passgen/pattern/kind.h"
#include "passgen/pattern/segment.h"
#include "passgen/pattern/range.h"

int passgen_parse_start(struct parser *parser) {
  // set initial group
  parser_state_push_group(
      parser,
      &parser->pattern.group,
      passgen_pattern_group_new_segment(&parser->pattern.group));

  return 0;
}

int passgen_parse_token(struct parser *parser, struct passgen_token *token) {
  struct parser_state *state =
    passgen_parser_get_state_last(parser);

  switch(state->type) {
    case PARSER_GROUP:
      return passgen_parse_group(parser, token, state);
    case PARSER_SET:
      return passgen_parse_set(parser, token, state);
    case PARSER_SET_RANGE:
      return passgen_parse_set_range(parser, token, state);
    default:
      return -1;
  }
}

int passgen_parse_group(
    struct parser *parser,
    struct passgen_token *token,
    struct parser_state *state) {
  // create new segment and parser state
  if(token->codepoint == '|') {
    state->data.group.segment =
        passgen_pattern_group_new_segment(state->data.group.group);
    return 0;
  }

  // this group's over
  if(token->codepoint == ')') {
    parser_state_pop(parser);
    return 0;
  }

  // we're supposed to read something in.
  if(token->codepoint == '(') {
    struct passgen_pattern_group *group =
        passgen_pattern_segment_new_group(state->data.group.segment);
    parser_state_push_group(
        parser,
        group,
        passgen_pattern_group_new_segment(group));
    return 0;
  }

  if(token->codepoint == '[') {
    struct passgen_pattern_set *set =
        passgen_pattern_segment_new_set(state->data.group.segment);
    parser_state_push_set(parser, set, NULL);
    return 0;
  }

  struct passgen_pattern_char *chr =
      passgen_pattern_segment_new_char(state->data.group.segment);
  chr->codepoint = token->codepoint;
  chr->repeat.min = 1;
  chr->repeat.max = 1;

  return 0;
}

int passgen_parse_set(
    struct parser *parser,
    struct passgen_token *token,
    struct parser_state *state) {
  // this set's over
  if(token->codepoint == ']') {
    parser_state_pop(parser);
    return 0;
  }

  // part of a range expression
  if(token->codepoint == '-') {
    state->type = PARSER_SET_RANGE;
    return 0;
  }

  struct passgen_pattern_range *range = passgen_pattern_set_new_range(state->data.set.set);

  range->start = token->codepoint;
  range->end = token->codepoint;

  state->data.set.range = range;

  return 0;
}

int passgen_parse_set_range(
    struct parser *parser,
    struct passgen_token *token,
    struct parser_state *state) {
  state->data.set.range->end = token->codepoint;
  state->type = PARSER_SET;

  return 0;
}

int parse_finish(struct parser *parser) {
  return 0;
}
