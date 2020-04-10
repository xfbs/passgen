#include "passgen/parser.h"
#include "passgen/pattern/item.h"
#include "passgen/pattern/kind.h"
#include "passgen/pattern/segment.h"

int passgen_parse_start(struct parser *parser) {
  // set initial group
  parser_state_push_group(
      parser,
      &parser->pattern.group,
      passgen_pattern_group_new_segment(&parser->pattern.group));

  return 0;
}

int passgen_parse_token(struct parser *parser, struct passgen_token *token) {
  int ret;

  do {
    // get current state
    struct parser_state *state = passgen_array_get(
        &parser->state,
        sizeof(struct parser_state),
        parser->state.len - 1);

    switch(state->type) {
      case PARSER_GROUP: ret = passgen_parse_token_group(parser, token, state); break;
      default: return -1;
    }
  } while(ret > 0);

  return ret;
}

int passgen_parse_token_group(
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
  }

  struct passgen_pattern_char *chr =
      passgen_pattern_segment_new_char(state->data.group.segment);
  chr->codepoint = token->codepoint;

  return 0;
}

int parse_finish(struct parser *parser) {
  return 0;
}
