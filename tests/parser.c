#include "passgen/parser.h"
#include "passgen/data/pattern.h"
#include "passgen/data/segment.h"
#include "passgen/data/segment_item.h"
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
  struct parser parser = {0};
  struct passgen_pattern_segment *segment;
  passgen_parser_init(&parser);
  assert(0 == passgen_parse_start(&parser));

  // single empty segment
  assert(1 == parser.pattern.group.segments.len);
  segment = passgen_pattern_group_get_segment(&parser.pattern.group, 0);
  assert(0 == segment->items.len);

  passgen_parser_free(&parser);

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

  passgen_parser_free(&parser);

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

  passgen_parser_free(&parser);

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

  passgen_parser_free(&parser);

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
  assert(item->repeat.min == 1);
  assert(item->repeat.max == 1);

  passgen_parser_free(&parser);

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

  passgen_parser_free(&parser);

  return test_ok;
}

test_result test_parser_set_simple(void) {
  PREAMBLE();
  PARSE_CODEPOINT('[');
  PARSE_CODEPOINT('a');
  PARSE_CODEPOINT('b');
  PARSE_CODEPOINT(']');
  struct passgen_pattern_range *range;

  // single segment containing char 'a'
  assert(1 == parser.pattern.group.segments.len);

  segment = passgen_pattern_group_get_segment(&parser.pattern.group, 0);
  assert(segment);
  assert(1 == segment->items.len);

  item = passgen_pattern_segment_get_item(segment, 0);
  assert(item);
  assert(item->kind == PASSGEN_PATTERN_SET);

  assert(item->data.set.items.len == 2);

  range = passgen_pattern_set_get_range(&item->data.set, 0);
  assert(range);
  assert(range->start == 'a');
  assert(range->end == 'a');

  range = passgen_pattern_set_get_range(&item->data.set, 1);
  assert(range);
  assert(range->start == 'b');
  assert(range->end == 'b');

  passgen_parser_free(&parser);

  return test_ok;
}

test_result test_parser_range_simple(void) {
  PREAMBLE();
  PARSE_CODEPOINT('[');
  PARSE_CODEPOINT('a');
  PARSE_CODEPOINT('-');
  PARSE_CODEPOINT('b');
  PARSE_CODEPOINT(']');
  struct passgen_pattern_range *range;

  // single segment containing char 'a'
  assert(1 == parser.pattern.group.segments.len);

  segment = passgen_pattern_group_get_segment(&parser.pattern.group, 0);
  assert(segment);
  assert(1 == segment->items.len);

  item = passgen_pattern_segment_get_item(segment, 0);
  assert(item);
  assert(item->kind == PASSGEN_PATTERN_SET);

  assert(item->data.set.items.len == 1);

  range = passgen_pattern_set_get_range(&item->data.set, 0);
  assert(range);
  assert(range->start == 'a');
  assert(range->end == 'b');

  passgen_parser_free(&parser);

  return test_ok;
}

test_result test_parser_range_multiple(void) {
  PREAMBLE();
  PARSE_CODEPOINT('[');
  PARSE_CODEPOINT('a');
  PARSE_CODEPOINT('-');
  PARSE_CODEPOINT('b');
  PARSE_CODEPOINT('c');
  PARSE_CODEPOINT('-');
  PARSE_CODEPOINT('d');
  PARSE_CODEPOINT(']');
  struct passgen_pattern_range *range;

  // single segment containing char 'a'
  assert(1 == parser.pattern.group.segments.len);

  segment = passgen_pattern_group_get_segment(&parser.pattern.group, 0);
  assert(segment);
  assert(1 == segment->items.len);

  item = passgen_pattern_segment_get_item(segment, 0);
  assert(item);
  assert(item->kind == PASSGEN_PATTERN_SET);

  assert(item->data.set.items.len == 2);

  range = passgen_pattern_set_get_range(&item->data.set, 0);
  assert(range);
  assert(range->start == 'a');
  assert(range->end == 'b');

  range = passgen_pattern_set_get_range(&item->data.set, 1);
  assert(range);
  assert(range->start == 'c');
  assert(range->end == 'd');

  passgen_parser_free(&parser);

  return test_ok;
}

test_result test_parser_char_repeat(void) {
  PREAMBLE();
  PARSE_CODEPOINT('a');
  PARSE_CODEPOINT('{');
  PARSE_CODEPOINT('2');
  PARSE_CODEPOINT('}');

  // single segment containing char 'a'
  assert(1 == parser.pattern.group.segments.len);

  segment = passgen_pattern_group_get_segment(&parser.pattern.group, 0);
  assert(segment);
  assert(1 == segment->items.len);

  item = passgen_pattern_segment_get_item(segment, 0);
  assert(item);
  assert(item->kind == PASSGEN_PATTERN_CHAR);
  assert(item->data.character.codepoint == 'a');
  assert(item->repeat.min == 2);
  assert(item->repeat.max == 2);

  passgen_parser_free(&parser);

  return test_ok;
}

test_result test_parser_char_repeat_range(void) {
  PREAMBLE();
  PARSE_CODEPOINT('a');
  PARSE_CODEPOINT('{');
  PARSE_CODEPOINT('2');
  PARSE_CODEPOINT(',');
  PARSE_CODEPOINT('4');
  PARSE_CODEPOINT('}');

  // single segment containing char 'a'
  assert(1 == parser.pattern.group.segments.len);

  segment = passgen_pattern_group_get_segment(&parser.pattern.group, 0);
  assert(segment);
  assert(1 == segment->items.len);

  item = passgen_pattern_segment_get_item(segment, 0);
  assert(item);
  assert(item->kind == PASSGEN_PATTERN_CHAR);
  assert(item->data.character.codepoint == 'a');
  assert(item->repeat.min == 2);
  assert(item->repeat.max == 4);

  passgen_parser_free(&parser);

  return test_ok;
}
