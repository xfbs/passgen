#include "passgen/parser.h"
#include "passgen/pattern/item.h"
#include "tests.h"
#include <stdbool.h>
#include <stdio.h>
#include <time.h>

#define PREAMBLE()                                \
  struct parser parser = {0};                     \
  struct passgen_token_parser token_parser = {0}; \
  struct passgen_token token = {0};               \
  struct passgen_pattern_segment *segment;        \
  struct passgen_pattern_item *item;              \
  passgen_parser_init(&parser);                   \
  assert(0 == passgen_parse_start(&parser))

#define PARSE_CODEPOINT(codepoint)                             \
  assert(                                                      \
      passgen_token_parse(&token_parser, &token, codepoint) == \
      PASSGEN_TOKEN_INIT);                                     \
  assert(0 == passgen_parse_token(&parser, &token))

test_result test_parser_empty(void) {
  PREAMBLE();

  // single empty segment
  assert(1 == parser.pattern.group.segments.len);
  segment = passgen_pattern_group_get_segment(&parser.pattern.group, 0);
  assert(0 == segment->items.len);

  return test_ok;
}

test_result test_parser_single_char(void) {
  PREAMBLE();
  PARSE_CODEPOINT('a');

  // single segment containing char 'a'
  assert(1 == parser.pattern.group.segments.len);

  segment = passgen_pattern_group_get_segment(&parser.pattern.group, 0);
  assert(segment);
  assert(1 == segment->items.len);

  item = passgen_pattern_segment_get_item(segment, 0);
  assert(item);
  assert(item->kind == PASSGEN_PATTERN_CHAR);
  assert(item->data.character.codepoint == 'a');

  return test_ok;
}

test_result test_parser_multi_char(void) {
  PREAMBLE();
  PARSE_CODEPOINT('a');
  PARSE_CODEPOINT('b');

  // single segment containing char 'a'
  assert(1 == parser.pattern.group.segments.len);

  segment = passgen_pattern_group_get_segment(&parser.pattern.group, 0);
  assert(segment);
  assert(2 == segment->items.len);

  item = passgen_pattern_segment_get_item(segment, 0);
  assert(item);
  assert(item->kind == PASSGEN_PATTERN_CHAR);
  assert(item->data.character.codepoint == 'a');

  item = passgen_pattern_segment_get_item(segment, 1);
  assert(item);
  assert(item->kind == PASSGEN_PATTERN_CHAR);
  assert(item->data.character.codepoint == 'b');

  return test_ok;
}

test_result test_parser_multi_groups(void) {
  PREAMBLE();
  PARSE_CODEPOINT('a');
  PARSE_CODEPOINT('|');
  PARSE_CODEPOINT('b');

  assert(2 == parser.pattern.group.segments.len);

  segment = passgen_pattern_group_get_segment(&parser.pattern.group, 0);
  assert(1 == segment->items.len);

  item = passgen_pattern_segment_get_item(segment, 0);
  assert(item);
  assert(item->kind == PASSGEN_PATTERN_CHAR);
  assert(item->data.character.codepoint == 'a');

  segment = passgen_pattern_group_get_segment(&parser.pattern.group, 1);
  assert(1 == segment->items.len);

  item = passgen_pattern_segment_get_item(segment, 0);
  assert(item);
  assert(item->kind == PASSGEN_PATTERN_CHAR);
  assert(item->data.character.codepoint == 'b');

  return test_ok;
}

test_result test_parser_nested_groups(void) {
  PREAMBLE();
  PARSE_CODEPOINT('(');
  PARSE_CODEPOINT('a');
  PARSE_CODEPOINT(')');

  assert(1 == parser.pattern.group.segments.len);

  segment = passgen_pattern_group_get_segment(&parser.pattern.group, 0);
  assert(1 == segment->items.len);

  item = passgen_pattern_segment_get_item(segment, 0);
  assert(item);
  assert(item->kind == PASSGEN_PATTERN_GROUP);

  assert(1 == item->data.group.segments.len);

  segment = passgen_pattern_group_get_segment(&item->data.group, 0);
  assert(1 == segment->items.len);

  item = passgen_pattern_segment_get_item(segment, 0);
  assert(item);
  assert(item->kind == PASSGEN_PATTERN_CHAR);
  assert(item->data.character.codepoint == 'a');

  return test_ok;
}

test_result test_parser_multi_nested_groups(void) {
  PREAMBLE();
  PARSE_CODEPOINT('(');
  PARSE_CODEPOINT('a');
  PARSE_CODEPOINT(')');
  PARSE_CODEPOINT('(');
  PARSE_CODEPOINT('b');
  PARSE_CODEPOINT(')');

  assert(1 == parser.pattern.group.segments.len);

  segment = passgen_pattern_group_get_segment(&parser.pattern.group, 0);
  assert(2 == segment->items.len);

  item = passgen_pattern_segment_get_item(segment, 0);
  assert(item);
  assert(item->kind == PASSGEN_PATTERN_GROUP);

  assert(1 == item->data.group.segments.len);

  segment = passgen_pattern_group_get_segment(&item->data.group, 0);
  assert(1 == segment->items.len);

  item = passgen_pattern_segment_get_item(segment, 0);
  assert(item);
  assert(item->kind == PASSGEN_PATTERN_CHAR);
  assert(item->data.character.codepoint == 'a');

  segment = passgen_pattern_group_get_segment(&parser.pattern.group, 0);
  item = passgen_pattern_segment_get_item(segment, 1);
  assert(item);
  assert(item->kind == PASSGEN_PATTERN_GROUP);

  assert(1 == item->data.group.segments.len);

  segment = passgen_pattern_group_get_segment(&item->data.group, 0);
  assert(1 == segment->items.len);

  item = passgen_pattern_segment_get_item(segment, 0);
  assert(item);
  assert(item->kind == PASSGEN_PATTERN_CHAR);
  assert(item->data.character.codepoint == 'b');

  return test_ok;
}

test_result test_parser_set_simple(void) {
  PREAMBLE();
  PARSE_CODEPOINT('[');
  PARSE_CODEPOINT('a');
  PARSE_CODEPOINT('b');
  PARSE_CODEPOINT(']');

  // single segment containing char 'a'
  assert(1 == parser.pattern.group.segments.len);

  segment = passgen_pattern_group_get_segment(&parser.pattern.group, 0);
  assert(segment);
  assert(1 == segment->items.len);

  item = passgen_pattern_segment_get_item(segment, 0);
  assert(item);
  assert(item->kind == PASSGEN_PATTERN_SET);

  return test_ok;
}
