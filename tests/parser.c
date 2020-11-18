#include "passgen/parser.h"

#include "passgen/container/stack/range.h"
#include "passgen/container/stack/segment.h"
#include "passgen/container/stack/segment_item.h"
#include "passgen/data/char.h"
#include "passgen/data/group.h"
#include "passgen/data/parser.h"
#include "passgen/data/pattern.h"
#include "passgen/data/pattern_kind.h"
#include "passgen/data/range.h"
#include "passgen/data/repeat.h"
#include "passgen/data/segment.h"
#include "passgen/data/segment_item.h"
#include "passgen/data/set.h"
#include "passgen/data/token.h"
#include "passgen/token.h"
#include "tests.h"

#define PREAMBLE()                                   \
     struct passgen_parser parser;                   \
     struct passgen_token_parser token_parser = {0}; \
     struct passgen_token token = {0};               \
     struct passgen_pattern_segment *segment;        \
     struct passgen_pattern_item *item;              \
     passgen_parser_init(&parser);                   \
     assert(0 == passgen_parse_start(&parser))

#define POSTAMBLE()                              \
     assert(0 == passgen_parse_finish(&parser)); \
     passgen_parser_free(&parser)

#define PARSE_CODEPOINT(codepoint)                                \
     assert(                                                      \
         passgen_token_parse(&token_parser, &token, codepoint) == \
         PASSGEN_TOKEN_INIT);                                     \
     assert(0 == passgen_parse_token(&parser, &token))

#define PARSE_CODEPOINT_DOUBLE(a, b)                                           \
     assert(passgen_token_parse(&token_parser, &token, a) > 0);                \
     assert(                                                                   \
         passgen_token_parse(&token_parser, &token, b) == PASSGEN_TOKEN_INIT); \
     assert(0 == passgen_parse_token(&parser, &token))

test_result test_parser_empty(void) {
     struct passgen_parser parser;
     struct passgen_pattern_segment *segment;
     passgen_parser_init(&parser);
     assert(0 == passgen_parse_start(&parser));

     // single empty segment
     assert(1 == parser.pattern.group.segments.len);
     segment = passgen_pattern_group_get_segment(&parser.pattern.group, 0);
     assert(0 == segment->items.len);

     POSTAMBLE();

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
     assert(item->data.character.codepoints[0] == 'a');
     assert(item->data.character.count == 1);

     POSTAMBLE();

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
     assert(1 == segment->items.len);

     item = passgen_pattern_segment_get_item(segment, 0);
     assert(item);
     assert(item->kind == PASSGEN_PATTERN_CHAR);
     assert(item->data.character.codepoints[0] == 'a');
     assert(item->data.character.codepoints[1] == 'b');
     assert(item->data.character.count == 2);

     POSTAMBLE();

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
     assert(item->data.character.codepoints[0] == 'a');
     assert(item->data.character.count == 1);

     segment = passgen_pattern_group_get_segment(&parser.pattern.group, 1);
     assert(1 == segment->items.len);

     item = passgen_pattern_segment_get_item(segment, 0);
     assert(item);
     assert(item->kind == PASSGEN_PATTERN_CHAR);
     assert(item->data.character.codepoints[0] == 'b');
     assert(item->data.character.count == 1);

     POSTAMBLE();

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
     assert(item->data.character.codepoints[0] == 'a');
     assert(item->data.character.count == 1);
     assert(item->repeat.min == 1);
     assert(item->repeat.max == 1);

     POSTAMBLE();

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
     assert(item->data.character.codepoints[0] == 'a');
     assert(item->data.character.count == 1);

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
     assert(item->data.character.codepoints[0] == 'b');
     assert(item->data.character.count == 1);

     POSTAMBLE();

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

     POSTAMBLE();

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

     POSTAMBLE();

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

     POSTAMBLE();

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
     assert(item->data.character.codepoints[0] == 'a');
     assert(item->data.character.count == 1);
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
     assert(item->data.character.codepoints[0] == 'a');
     assert(item->data.character.count == 1);
     assert(item->repeat.min == 2);
     assert(item->repeat.max == 4);

     POSTAMBLE();

     return test_ok;
}

test_result test_parser_group_ignore_escaped(void) {
     PREAMBLE();
     PARSE_CODEPOINT_DOUBLE('\\', '(');
     PARSE_CODEPOINT_DOUBLE('\\', '{');
     PARSE_CODEPOINT_DOUBLE('\\', '[');
     PARSE_CODEPOINT_DOUBLE('\\', '|');

     assert(1 == parser.pattern.group.segments.len);

     segment = passgen_pattern_group_get_segment(&parser.pattern.group, 0);
     assert(segment);
     assert(1 == segment->items.len);

     item = passgen_pattern_segment_get_item(segment, 0);
     assert(item);
     assert(item->kind == PASSGEN_PATTERN_CHAR);
     assert(item->data.character.codepoints[0] == '(');
     assert(item->data.character.codepoints[1] == '{');
     assert(item->data.character.codepoints[2] == '[');
     assert(item->data.character.codepoints[3] == '|');
     assert(item->data.character.count == 4);
     assert(item->repeat.min == 1);
     assert(item->repeat.max == 1);

     POSTAMBLE();

     return test_ok;
}

test_result test_parser_item_maybe(void) {
     PREAMBLE();
     PARSE_CODEPOINT('a');
     PARSE_CODEPOINT('a');
     PARSE_CODEPOINT('?');
     PARSE_CODEPOINT('(');
     PARSE_CODEPOINT(')');
     PARSE_CODEPOINT('(');
     PARSE_CODEPOINT(')');
     PARSE_CODEPOINT('?');

     // single segment containing char 'a'
     assert(1 == parser.pattern.group.segments.len);

     segment = passgen_pattern_group_get_segment(&parser.pattern.group, 0);
     assert(segment);
     assert(4 == segment->items.len);

     item = passgen_pattern_segment_get_item(segment, 0);
     assert(item);
     assert(item->kind == PASSGEN_PATTERN_CHAR);
     assert(item->data.character.codepoints[0] == 'a');
     assert(item->data.character.count == 1);
     assert(item->repeat.min == 1);
     assert(item->repeat.max == 1);
     assert(item->maybe == false);

     item = passgen_pattern_segment_get_item(segment, 1);
     assert(item);
     assert(item->kind == PASSGEN_PATTERN_CHAR);
     assert(item->data.character.codepoints[0] == 'a');
     assert(item->data.character.count == 1);
     assert(item->repeat.min == 1);
     assert(item->repeat.max == 1);
     assert(item->maybe == true);

     item = passgen_pattern_segment_get_item(segment, 2);
     assert(item);
     assert(item->kind == PASSGEN_PATTERN_GROUP);
     assert(item->repeat.min == 1);
     assert(item->repeat.max == 1);
     assert(item->maybe == false);

     item = passgen_pattern_segment_get_item(segment, 3);
     assert(item);
     assert(item->kind == PASSGEN_PATTERN_GROUP);
     assert(item->repeat.min == 1);
     assert(item->repeat.max == 1);
     assert(item->maybe == true);

     POSTAMBLE();

     return test_ok;
}

test_result test_parser_special_pronounceable(void) {
     PREAMBLE();
     PARSE_CODEPOINT_DOUBLE('\\', 'p');
     PARSE_CODEPOINT('[');
     PARSE_CODEPOINT('e');
     PARSE_CODEPOINT('n');
     PARSE_CODEPOINT('g');
     PARSE_CODEPOINT('l');
     PARSE_CODEPOINT('i');
     PARSE_CODEPOINT('s');
     PARSE_CODEPOINT('h');
     PARSE_CODEPOINT(']');
     PARSE_CODEPOINT('{');
     PARSE_CODEPOINT('9');
     PARSE_CODEPOINT(',');
     PARSE_CODEPOINT('1');
     PARSE_CODEPOINT('2');
     PARSE_CODEPOINT('}');

     assert(parser.state.len == 1);

     assert(1 == parser.pattern.group.segments.len);

     segment = passgen_pattern_group_get_segment(&parser.pattern.group, 0);
     assert(segment);
     assert(1 == segment->items.len);

     item = passgen_pattern_segment_get_item(segment, 0);
     assert(item);
     assert(item->kind == PASSGEN_PATTERN_SPECIAL);
     assert(item->repeat.min == 1);
     assert(item->repeat.max == 1);
     assert(item->maybe == false);
     assert(item->data.special.kind == PASSGEN_PATTERN_SPECIAL_PRONOUNCABLE);
     assert(item->data.special.length.min == 9);
     assert(item->data.special.length.max == 12);

     POSTAMBLE();

     return test_ok;
}

test_result test_parser_mixed_special(void) {
     PREAMBLE();
     PARSE_CODEPOINT('a');
     PARSE_CODEPOINT_DOUBLE('\\', 'p');
     PARSE_CODEPOINT('[');
     PARSE_CODEPOINT('e');
     PARSE_CODEPOINT('n');
     PARSE_CODEPOINT('g');
     PARSE_CODEPOINT('l');
     PARSE_CODEPOINT('i');
     PARSE_CODEPOINT('s');
     PARSE_CODEPOINT('h');
     PARSE_CODEPOINT(']');
     PARSE_CODEPOINT('{');
     PARSE_CODEPOINT('9');
     PARSE_CODEPOINT(',');
     PARSE_CODEPOINT('1');
     PARSE_CODEPOINT('2');
     PARSE_CODEPOINT('}');
     PARSE_CODEPOINT('x');

     assert(parser.state.len == 1);

     assert(1 == parser.pattern.group.segments.len);

     segment = passgen_pattern_group_get_segment(&parser.pattern.group, 0);
     assert(segment);
     assert(3 == segment->items.len);

     item = passgen_pattern_segment_get_item(segment, 0);
     assert(item);
     assert(item->kind == PASSGEN_PATTERN_CHAR);
     assert(item->data.character.codepoints[0] == 'a');
     assert(item->data.character.count == 1);
     assert(item->repeat.min == 1);
     assert(item->repeat.max == 1);
     assert(item->maybe == false);

     item = passgen_pattern_segment_get_item(segment, 1);
     assert(item);
     assert(item->kind == PASSGEN_PATTERN_SPECIAL);
     assert(item->repeat.min == 1);
     assert(item->repeat.max == 1);
     assert(item->maybe == false);
     assert(item->data.special.kind == PASSGEN_PATTERN_SPECIAL_PRONOUNCABLE);
     assert(item->data.special.length.min == 9);
     assert(item->data.special.length.max == 12);

     item = passgen_pattern_segment_get_item(segment, 2);
     assert(item);
     assert(item->kind == PASSGEN_PATTERN_CHAR);
     assert(item->data.character.codepoints[0] == 'x');
     assert(item->data.character.count == 1);
     assert(item->repeat.min == 1);
     assert(item->repeat.max == 1);
     assert(item->maybe == false);

     POSTAMBLE();

     return test_ok;
}

test_result test_parser_char_maybe_char(void) {
     PREAMBLE();
     PARSE_CODEPOINT('a');
     PARSE_CODEPOINT('b');
     PARSE_CODEPOINT('c');
     PARSE_CODEPOINT('?');
     PARSE_CODEPOINT('d');

     assert(parser.state.len == 1);

     assert(1 == parser.pattern.group.segments.len);

     segment = passgen_pattern_group_get_segment(&parser.pattern.group, 0);
     assert(segment);
     assert(3 == segment->items.len);

     item = passgen_pattern_segment_get_item(segment, 0);
     assert(item);
     assert(item->kind == PASSGEN_PATTERN_CHAR);
     assert(item->data.character.count == 2);
     assert(item->data.character.codepoints[0] == 'a');
     assert(item->data.character.codepoints[1] == 'b');
     assert(item->data.character.tainted == false);
     assert(item->repeat.min == 1);
     assert(item->repeat.max == 1);
     assert(item->maybe == false);

     item = passgen_pattern_segment_get_item(segment, 1);
     assert(item);
     assert(item->kind == PASSGEN_PATTERN_CHAR);
     assert(item->data.character.count == 1);
     assert(item->data.character.codepoints[0] == 'c');
     assert(item->data.character.tainted == true);
     assert(item->repeat.min == 1);
     assert(item->repeat.max == 1);
     assert(item->maybe == true);

     item = passgen_pattern_segment_get_item(segment, 2);
     assert(item);
     assert(item->kind == PASSGEN_PATTERN_CHAR);
     assert(item->data.character.count == 1);
     assert(item->data.character.codepoints[0] == 'd');
     assert(item->data.character.tainted == false);
     assert(item->repeat.min == 1);
     assert(item->repeat.max == 1);
     assert(item->maybe == false);

     POSTAMBLE();

     return test_ok;
}

test_result test_parser_char_repeat_tainted(void) {
     PREAMBLE();
     PARSE_CODEPOINT('a');
     PARSE_CODEPOINT('{');
     PARSE_CODEPOINT('3');
     PARSE_CODEPOINT('}');
     PARSE_CODEPOINT('b');
     PARSE_CODEPOINT('c');
     PARSE_CODEPOINT('d');
     PARSE_CODEPOINT('e');
     PARSE_CODEPOINT('f');
     PARSE_CODEPOINT('g');
     PARSE_CODEPOINT('h');
     PARSE_CODEPOINT('i');
     PARSE_CODEPOINT('j');
     PARSE_CODEPOINT('k');
     PARSE_CODEPOINT('{');
     PARSE_CODEPOINT('2');
     PARSE_CODEPOINT('}');
     PARSE_CODEPOINT('?');
     PARSE_CODEPOINT('a');
     PARSE_CODEPOINT('b');

     assert(parser.state.len == 1);

     assert(1 == parser.pattern.group.segments.len);

     segment = passgen_pattern_group_get_segment(&parser.pattern.group, 0);
     assert(segment);
     assert(5 == segment->items.len);

     item = passgen_pattern_segment_get_item(segment, 0);
     assert(item);
     assert(item->kind == PASSGEN_PATTERN_CHAR);
     assert(item->data.character.count == 1);
     assert(item->data.character.codepoints[0] == 'a');
     assert(item->data.character.tainted == true);
     assert(item->repeat.min == 3);
     assert(item->repeat.max == 3);
     assert(item->maybe == false);

     item = passgen_pattern_segment_get_item(segment, 1);
     assert(item);
     assert(item->kind == PASSGEN_PATTERN_CHAR);
     assert(item->data.character.count == 7);
     assert(item->data.character.codepoints[0] == 'b');
     assert(item->data.character.codepoints[1] == 'c');
     assert(item->data.character.codepoints[2] == 'd');
     assert(item->data.character.codepoints[3] == 'e');
     assert(item->data.character.codepoints[4] == 'f');
     assert(item->data.character.codepoints[5] == 'g');
     assert(item->data.character.codepoints[6] == 'h');
     assert(item->data.character.tainted == false);
     assert(item->repeat.min == 1);
     assert(item->repeat.max == 1);
     assert(item->maybe == false);

     item = passgen_pattern_segment_get_item(segment, 2);
     assert(item);
     assert(item->kind == PASSGEN_PATTERN_CHAR);
     assert(item->data.character.count == 2);
     assert(item->data.character.codepoints[0] == 'i');
     assert(item->data.character.codepoints[1] == 'j');
     assert(item->data.character.tainted == false);
     assert(item->repeat.min == 1);
     assert(item->repeat.max == 1);
     assert(item->maybe == false);

     item = passgen_pattern_segment_get_item(segment, 3);
     assert(item);
     assert(item->kind == PASSGEN_PATTERN_CHAR);
     assert(item->data.character.count == 1);
     assert(item->data.character.codepoints[0] == 'k');
     assert(item->data.character.tainted == true);
     assert(item->repeat.min == 2);
     assert(item->repeat.max == 2);
     assert(item->maybe == true);

     item = passgen_pattern_segment_get_item(segment, 4);
     assert(item);
     assert(item->kind == PASSGEN_PATTERN_CHAR);
     assert(item->data.character.count == 2);
     assert(item->data.character.codepoints[0] == 'a');
     assert(item->data.character.codepoints[1] == 'b');
     assert(item->data.character.tainted == false);
     assert(item->repeat.min == 1);
     assert(item->repeat.max == 1);
     assert(item->maybe == false);

     POSTAMBLE();

     return test_ok;
}
