#include "passgen/parser.h"
#include "passgen/data/segment_item.h"
#include "passgen/data/pattern_kind.h"
#include "passgen/data/range.h"
#include "passgen/data/segment.h"

int passgen_parse_start(struct parser *parser) {
  // set initial group
  parser_state_push_group(
      parser,
      &parser->pattern.group,
      passgen_pattern_group_new_segment(&parser->pattern.group));

  return 0;
}

int passgen_parse_token(struct parser *parser, struct passgen_token *token) {
  struct passgen_parser_state *state = passgen_parser_get_state_last(parser);

  switch(state->type) {
    case PARSER_GROUP: return passgen_parse_group(parser, token, state);
    case PARSER_SET: return passgen_parse_set(parser, token, state);
    case PARSER_SET_RANGE: return passgen_parse_set_range(parser, token, state);
    case PARSER_REPEAT: return passgen_parse_repeat(parser, token, state);
    case PARSER_REPEAT_RANGE:
      return passgen_parse_repeat_range(parser, token, state);
    default: return -1;
  }
}

int passgen_parse_group(
    struct parser *parser,
    struct passgen_token *token,
    struct passgen_parser_state *state) {
  struct passgen_pattern_group *group;
  struct passgen_pattern_item *item;
  switch(token->codepoint) {
    case '|':
      // create new segment and parser state
      state->data.group.segment =
          passgen_pattern_group_new_segment(state->data.group.group);
      return 0;
    case ')': parser_state_pop(parser); return 0;
    case '(':
      // we're supposed to read something in.
      group = passgen_pattern_segment_new_group(state->data.group.segment);
      parser_state_push_group(
          parser,
          group,
          passgen_pattern_group_new_segment(group));
      return 0;
    case '[':
      parser_state_push_set(
          parser,
          passgen_pattern_segment_new_set(state->data.group.segment),
          NULL);
      return 0;
    case '{':
      item = passgen_pattern_segment_get_item(
          state->data.group.segment,
          state->data.group.segment->items.len - 1);
      // clear default repetition
      item->repeat.min = 0;
      item->repeat.max = 0;
      parser_state_push_repeat(parser, item);
      return 0;
    default: break;
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
    struct passgen_parser_state *state) {
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

  struct passgen_pattern_range *range =
      passgen_pattern_set_new_range(state->data.set.set);

  range->start = token->codepoint;
  range->end = token->codepoint;

  state->data.set.range = range;

  return 0;
}

int passgen_parse_set_range(
    struct parser *parser,
    struct passgen_token *token,
    struct passgen_parser_state *state) {
  state->data.set.range->end = token->codepoint;
  state->type = PARSER_SET;

  return 0;
}

int passgen_parse_repeat(
    struct parser *parser,
    struct passgen_token *token,
    struct passgen_parser_state *state) {
  // this set's over
  if(token->codepoint == '}') {
    state->data.repeat.item->repeat.max = state->data.repeat.item->repeat.min;
    parser_state_pop(parser);
    return 0;
  }

  if(token->codepoint == ',') {
    state->data.repeat.item->repeat.max = 0;
    state->type = PARSER_REPEAT_RANGE;
    return 0;
  }

  if(token->codepoint >= '0' && token->codepoint <= '9') {
    uint8_t digit = token->codepoint - '0';

    state->data.repeat.item->repeat.min *= 10;
    state->data.repeat.item->repeat.min += digit;

    return 0;
  }

  return -1;
}

int passgen_parse_repeat_range(
    struct parser *parser,
    struct passgen_token *token,
    struct passgen_parser_state *state) {
  if(token->codepoint == '}') {
    parser_state_pop(parser);
    return 0;
  }

  if(token->codepoint >= '0' && token->codepoint <= '9') {
    uint8_t digit = token->codepoint - '0';

    state->data.repeat.item->repeat.max *= 10;
    state->data.repeat.item->repeat.max += digit;

    return 0;
  }

  return -1;
}

int parse_finish(struct parser *parser) {
  return 0;
}
