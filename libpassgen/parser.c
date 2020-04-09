#include "passgen/parser.h"
#include "passgen/pattern/kind.h"
#include "passgen/pattern/item.h"
#include "passgen/pattern/segment.h"

static inline struct parser_state *parser_state_push(struct parser *parser) {
  return passgen_array_push(&parser->state, sizeof(struct parser_state), NULL);
}

static inline struct parser_state *parser_state_push_group(
    struct parser *parser,
    struct passgen_pattern_group *group,
    struct passgen_pattern_segment *segment) {
  struct parser_state *state = parser_state_push(parser);
  state->type = PARSER_GROUP;
  state->data.group.group = group;
  state->data.group.segment = segment;

  return state;
}

static inline struct parser_state *parser_state_push_set(
    struct parser *parser,
    struct passgen_pattern_set *set,
    struct passgen_pattern_range *range) {
  struct parser_state *state = parser_state_push(parser);
  state->type = PARSER_SET;
  state->data.set.set = set;
  state->data.set.range = range;

  return state;
}

static inline void parser_state_init(struct parser *parser) {
  passgen_array_init(&parser->state, sizeof(struct parser_state), NULL);
}

static inline void parser_state_pop(struct parser *parser) {
  passgen_array_pop(&parser->state, sizeof(struct parser_state), NULL);
}

int parser_init(struct parser *parser) {
  // initialise parsing stack
  parser_state_init(parser);

  // initialise group
  passgen_pattern_init(&parser->pattern);

  // set initial group
  parser_state_push_group(
      parser,
      &parser->pattern.group,
      passgen_pattern_group_new_segment(&parser->pattern.group));

  return 0;
}

int parse_token_group(
    struct parser *parser,
    struct passgen_token *token,
    struct parser_state *state);

int parse_token(struct parser *parser, struct passgen_token *token) {
  int ret;

  do {
    // get current state
    struct parser_state *state = passgen_array_get(
        &parser->state,
        sizeof(struct parser_state),
        parser->state.len - 1);

    switch(state->type) {
      case PARSER_GROUP: ret = parse_token_group(parser, token, state); break;
      default: return -1;
    }
  } while(ret > 0);

  return ret;
}

int parse_token_group(
    struct parser *parser,
    struct passgen_token *token,
    struct parser_state *state) {
  // create new segment and parser state
  if(token->codepoint == '|') {
    state->data.group.segment = passgen_pattern_group_new_segment(state->data.group.group);
    return 0;
  }

  // this group's over
  if(token->codepoint == ')') {
    parser_state_pop(parser);
    return 0;
  }

  // we're supposed to read something in.
  if(token->codepoint == '(') {
    struct passgen_pattern_group *group = passgen_pattern_segment_new_group(state->data.group.segment);
    parser_state_push_group(
        parser,
        group,
        passgen_pattern_group_new_segment(group));
    return 0;
  }

  if(token->codepoint == '[') {
    /*
    item->kind = PASSGEN_PATTERN_SET;
    struct parser_state *state = parser_state_push(parser);
    state->type = PARSER_SET;
    state->data.set.set = &item->data.set;
    */
  }

  struct passgen_pattern_char *chr = passgen_pattern_segment_new_char(state->data.group.segment);
  chr->codepoint = token->codepoint;

  return 0;
}

int parse_finish(struct parser *parser) {
  return 0;
}

struct parser_state *passgen_parser_get_state(struct parser *parser, size_t n) {
  return passgen_array_get(
      &parser->state,
      sizeof(struct parser_state),
      n);
}

struct parser_state *passgen_parser_get_state_last(struct parser *parser) {
  if(parser->state.len != 0) {
    return passgen_parser_get_state(parser, parser->state.len - 1);
  } else {
    return NULL;
  }
}
