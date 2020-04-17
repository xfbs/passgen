#include "passgen/parser.h"

#include <stddef.h>
#include <stdint.h>

#include "passgen/container/stack/range.h"
#include "passgen/container/stack/segment_item.h"
#include "passgen/data/char.h"
#include "passgen/data/group.h"
#include "passgen/data/parser.h"
#include "passgen/data/parser_state.h"
#include "passgen/data/pattern.h"
#include "passgen/data/range.h"
#include "passgen/data/repeat.h"
#include "passgen/data/segment.h"
#include "passgen/data/segment_item.h"
#include "passgen/data/set.h"
#include "passgen/data/token.h"
#include "passgen/memory.h"

int passgen_parse_start(struct passgen_parser *parser) {
  // set initial group
  parser_state_push_group(
      parser,
      &parser->pattern.group,
      passgen_pattern_group_new_segment(&parser->pattern.group));

  return 0;
}

int passgen_parse_token(
    struct passgen_parser *parser,
    struct passgen_token *token) {
  struct passgen_parser_state *state = passgen_parser_get_state_last(parser);

  switch(state->type) {
    case PASSGEN_PARSER_GROUP: return passgen_parse_group(parser, token, state);
    case PASSGEN_PARSER_SET: return passgen_parse_set(parser, token, state);
    case PASSGEN_PARSER_SET_RANGE:
      return passgen_parse_set_range(parser, token, state);
    case PASSGEN_PARSER_REPEAT:
      return passgen_parse_repeat(parser, token, state);
    case PASSGEN_PARSER_REPEAT_RANGE:
      return passgen_parse_repeat_range(parser, token, state);
    default: return -1;
  }
}

int passgen_parse_group(
    struct passgen_parser *parser,
    struct passgen_token *token,
    struct passgen_parser_state *state) {
  uint32_t codepoint = token->codepoint;
  struct passgen_pattern_group *group;
  struct passgen_pattern_special *special;
  struct passgen_pattern_item *item;

  if(codepoint & PASSGEN_TOKEN_ESCAPED_BIT) {
    switch((char) codepoint) {
      case '|':
      case '(':
      case ')':
      case '{':
      case '}':
      case '[':
      case ']':
        codepoint &= ~PASSGEN_TOKEN_ESCAPED_BIT;
        break;
      case 'p':
      case 'w':
        // special token
        special = passgen_pattern_segment_new_special(state->data.group.segment);
        special->kind = codepoint;
        parser_state_push_special(parser, special);
        break;
      default:
        // error
        break;
    }
  } else {
    switch((char) codepoint) {
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
  }

  struct passgen_pattern_char *chr =
      passgen_pattern_segment_new_char(state->data.group.segment);
  chr->codepoint = token->codepoint;
  chr->repeat.min = 1;
  chr->repeat.max = 1;

  return 0;
}

int passgen_parse_set(
    struct passgen_parser *parser,
    struct passgen_token *token,
    struct passgen_parser_state *state) {
  struct passgen_pattern_set *set = state->data.set.set;

  // this set's over
  if(token->codepoint == ']') {
    // compute sum of choices and choices list for binary search.
    size_t choices = 0;
    set->choices_list = passgen_malloc(NULL, sizeof(size_t) * set->items.len);
    for(size_t i = 0; i < set->items.len; i++) {
      struct passgen_pattern_range *range =
          passgen_pattern_range_stack_get(&set->items, i);
      choices += 1 + range->end - range->start;
      set->choices_list[i] = choices;
    }

    parser_state_pop(parser);
    return 0;
  }

  // part of a range expression
  if(token->codepoint == '-') {
    state->type = PASSGEN_PARSER_SET_RANGE;
    return 0;
  }

  struct passgen_pattern_range *range = passgen_pattern_set_new_range(set);

  range->start = token->codepoint;
  range->end = token->codepoint;

  state->data.set.range = range;

  return 0;
}

int passgen_parse_set_range(
    struct passgen_parser *parser,
    struct passgen_token *token,
    struct passgen_parser_state *state) {
  (void)parser;
  state->data.set.range->end = token->codepoint;
  state->type = PASSGEN_PARSER_SET;

  return 0;
}

int passgen_parse_repeat(
    struct passgen_parser *parser,
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
    state->type = PASSGEN_PARSER_REPEAT_RANGE;
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
    struct passgen_parser *parser,
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

int parse_finish(struct passgen_parser *parser) {
  // TODO: implement
  (void)parser;

  return 0;
}
