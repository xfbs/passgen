#include "passgen/parser/parser.h"
#include "passgen/container/stack.h"
#include "passgen/parser/token.h"
#include "passgen/pattern/group.h"
#include "passgen/pattern/literal.h"
#include "passgen/pattern/pattern.h"
#include "passgen/pattern/range.h"
#include "passgen/pattern/repeat.h"
#include "passgen/pattern/segment.h"
#include "passgen/pattern/segment_item.h"
#include "passgen/pattern/set.h"
#include "passgen/util/random.h"
#include "passgen/util/utf8.h"
#include "tests.h"
#include <passgen/passgen.h>
#include <stdlib.h>

#define PREAMBLE()                                 \
    passgen_parser parser;                         \
    passgen_token_parser token_parser = {0};       \
    passgen_token token = {0};                     \
    passgen_pattern_segment *segment;              \
    passgen_pattern_item *item;                    \
    passgen_pattern parsed_pattern;                \
    passgen_parser_init(&parser, &parsed_pattern); \
    (void) item;                                   \
    (void) segment

#define POSTAMBLE()                              \
    assert_eq(0, passgen_parse_finish(&parser)); \
    passgen_parser_free(&parser);                \
    passgen_pattern_free(&parsed_pattern);

#define PARSE_CODEPOINT(codepoint)                                  \
    assert(                                                         \
        passgen_token_parse(&token_parser, &token, 1, codepoint) == \
        PASSGEN_TOKEN_INIT);                                        \
    assert_eq(0, passgen_parse_token(&parser, &token))

#define PARSE_CODEPOINT_DOUBLE(a, b)                              \
    assert(passgen_token_parse(&token_parser, &token, 1, a) > 0); \
    assert(                                                       \
        passgen_token_parse(&token_parser, &token, 1, b) ==       \
        PASSGEN_TOKEN_INIT);                                      \
    assert_eq(0, passgen_parse_token(&parser, &token))

test_result test_parser_empty(void) {
    PREAMBLE();
    (void) item;
    (void) token;
    (void) token_parser;

    // single empty segment
    assert(1 == parser.pattern->group.segments.len);
    segment = passgen_pattern_group_segment_get(&parser.pattern->group, 0);
    assert(0 == segment->items.len);

    POSTAMBLE();

    return test_ok;
}

test_result test_parser_segment_multiplier(void) {
    PREAMBLE();
    PARSE_CODEPOINT('(');

    segment = passgen_pattern_group_segment_get(&parser.pattern->group, 0);
    item = passgen_pattern_segment_get_item(segment, 0);

    PARSE_CODEPOINT('{');
    PARSE_CODEPOINT('5');
    PARSE_CODEPOINT('}');

    assert_eq(item->data.group.segments.len, 1);
    assert_eq(item->data.group.multiplier_sum, 5);
    segment = passgen_pattern_group_segment_get(&item->data.group, 0);
    assert_eq(segment->multiplier, 5);

    PARSE_CODEPOINT('a');

    assert_eq(segment->items.len, 1);

    PARSE_CODEPOINT('|');

    assert_eq(item->data.group.segments.len, 2);
    assert_eq(item->data.group.multiplier_sum, 6);
    segment = passgen_pattern_group_segment_get(&item->data.group, 1);
    assert_eq(segment->multiplier, 1);

    PARSE_CODEPOINT('b');
    PARSE_CODEPOINT('c');

    // one item (two codepoints in a single literal)
    assert_eq(segment->items.len, 1);

    PARSE_CODEPOINT('|');

    assert_eq(item->data.group.segments.len, 3);
    assert_eq(item->data.group.multiplier_sum, 7);
    segment = passgen_pattern_group_segment_get(&item->data.group, 2);
    assert_eq(segment->multiplier, 1);

    PARSE_CODEPOINT('{');
    PARSE_CODEPOINT('3');
    PARSE_CODEPOINT('}');

    assert_eq(item->data.group.multiplier_sum, 9);
    assert_eq(segment->multiplier, 3);

    PARSE_CODEPOINT('c');

    assert_eq(segment->items.len, 1);

    PARSE_CODEPOINT(')');

    assert_eq(item->data.group.multiplier_sum, 9);

    POSTAMBLE();

    return test_ok;
}

// when parsing a group, skip over any segments that have a zero multiplier.
test_result test_parser_skip_zero_segment(void) {
    PREAMBLE();
    PARSE_CODEPOINT('(');

    segment = passgen_pattern_group_segment_get(&parser.pattern->group, 0);
    item = passgen_pattern_segment_get_item(segment, 0);
    assert_eq(item->data.group.segments.len, 1);

    for(size_t i = 0; i < 100; i++) {
        PARSE_CODEPOINT('{');
        PARSE_CODEPOINT('0');
        PARSE_CODEPOINT('}');
        PARSE_CODEPOINT('|');

        // because the multiplier is zero, this segment is removed
        assert_eq(item->data.group.segments.len, 1);
    }

    PARSE_CODEPOINT('{');
    PARSE_CODEPOINT('0');
    PARSE_CODEPOINT('}');
    PARSE_CODEPOINT(')');

    // final segment is also removed, leaving no segments
    assert_eq(item->data.group.segments.len, 0);

    POSTAMBLE();
    return test_ok;
}

test_result test_parser_empty_group(void) {
    PREAMBLE();
    PARSE_CODEPOINT('(');

    assert(1 == parser.pattern->group.segments.len);
    segment = passgen_pattern_group_segment_get(&parser.pattern->group, 0);
    assert(segment);
    assert(1 == segment->items.len);

    // group with one empty segment
    item = passgen_pattern_segment_get_item(segment, 0);
    assert(item);
    assert(item->kind == PASSGEN_PATTERN_GROUP);
    assert_eq(item->data.group.segments.len, 1);

    PARSE_CODEPOINT(')');

    assert(1 == parser.pattern->group.segments.len);
    segment = passgen_pattern_group_segment_get(&parser.pattern->group, 0);
    assert(segment);
    assert(1 == segment->items.len);

    for(size_t i = 0; i < 100; i++) {
        PARSE_CODEPOINT('(');
        PARSE_CODEPOINT(')');

        assert(1 == parser.pattern->group.segments.len);
        segment = passgen_pattern_group_segment_get(&parser.pattern->group, 0);
        assert(segment);
        assert(1 == segment->items.len);
    }

    POSTAMBLE();
    return test_ok;
}

test_result test_parser_single_char(void) {
    PREAMBLE();
    PARSE_CODEPOINT('a');

    // single segment containing char 'a'
    assert(1 == parser.pattern->group.segments.len);

    segment = passgen_pattern_group_segment_get(&parser.pattern->group, 0);
    assert(segment);
    assert(1 == segment->items.len);

    item = passgen_pattern_segment_get_item(segment, 0);
    assert(item);
    assert(item->kind == PASSGEN_PATTERN_LITERAL);
    assert(item->data.literal.codepoints[0] == 'a');
    assert(item->data.literal.count == 1);

    POSTAMBLE();

    return test_ok;
}

test_result test_parser_multi_char(void) {
    PREAMBLE();
    PARSE_CODEPOINT('a');
    PARSE_CODEPOINT('b');

    // single segment containing char 'a'
    assert(1 == parser.pattern->group.segments.len);

    segment = passgen_pattern_group_segment_get(&parser.pattern->group, 0);
    assert(segment);
    assert(1 == segment->items.len);

    item = passgen_pattern_segment_get_item(segment, 0);
    assert(item);
    assert(item->kind == PASSGEN_PATTERN_LITERAL);
    assert(item->data.literal.codepoints[0] == 'a');
    assert(item->data.literal.codepoints[1] == 'b');
    assert(item->data.literal.count == 2);

    POSTAMBLE();

    return test_ok;
}

test_result test_parser_multi_groups(void) {
    PREAMBLE();
    PARSE_CODEPOINT('a');
    PARSE_CODEPOINT('|');
    PARSE_CODEPOINT('b');

    assert(2 == parser.pattern->group.segments.len);

    segment = passgen_pattern_group_segment_get(&parser.pattern->group, 0);
    assert(1 == segment->items.len);

    item = passgen_pattern_segment_get_item(segment, 0);
    assert(item);
    assert(item->kind == PASSGEN_PATTERN_LITERAL);
    assert(item->data.literal.codepoints[0] == 'a');
    assert(item->data.literal.count == 1);

    segment = passgen_pattern_group_segment_get(&parser.pattern->group, 1);
    assert(1 == segment->items.len);

    item = passgen_pattern_segment_get_item(segment, 0);
    assert(item);
    assert(item->kind == PASSGEN_PATTERN_LITERAL);
    assert(item->data.literal.codepoints[0] == 'b');
    assert(item->data.literal.count == 1);

    POSTAMBLE();

    return test_ok;
}

test_result test_parser_nested_groups(void) {
    PREAMBLE();
    PARSE_CODEPOINT('(');
    PARSE_CODEPOINT('a');
    PARSE_CODEPOINT(')');

    assert(1 == parser.pattern->group.segments.len);

    segment = passgen_pattern_group_segment_get(&parser.pattern->group, 0);
    assert(1 == segment->items.len);

    item = passgen_pattern_segment_get_item(segment, 0);
    assert(item);
    assert(item->kind == PASSGEN_PATTERN_GROUP);

    assert(1 == item->data.group.segments.len);

    segment = passgen_pattern_group_segment_get(&item->data.group, 0);
    assert(1 == segment->items.len);

    item = passgen_pattern_segment_get_item(segment, 0);
    assert(item);
    assert(item->kind == PASSGEN_PATTERN_LITERAL);
    assert(item->data.literal.codepoints[0] == 'a');
    assert(item->data.literal.count == 1);
    assert(item->repeat.min == 1);
    assert(item->repeat.max == 1);

    POSTAMBLE();

    return test_ok;
}

test_result test_parser_depth_limit(void) {
    PREAMBLE();
    parser.limit = 5;
    PARSE_CODEPOINT('(');
    PARSE_CODEPOINT('(');
    PARSE_CODEPOINT('(');
    PARSE_CODEPOINT('(');
    assert(0 != passgen_parse_token(&parser, &token));

    assert(0 != passgen_parse_finish(&parser));
    passgen_parser_free(&parser);
    passgen_pattern_free(&parsed_pattern);

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

    assert(1 == parser.pattern->group.segments.len);

    segment = passgen_pattern_group_segment_get(&parser.pattern->group, 0);
    assert(2 == segment->items.len);

    item = passgen_pattern_segment_get_item(segment, 0);
    assert(item);
    assert(item->kind == PASSGEN_PATTERN_GROUP);

    assert(1 == item->data.group.segments.len);

    segment = passgen_pattern_group_segment_get(&item->data.group, 0);
    assert(1 == segment->items.len);

    item = passgen_pattern_segment_get_item(segment, 0);
    assert(item);
    assert(item->kind == PASSGEN_PATTERN_LITERAL);
    assert(item->data.literal.codepoints[0] == 'a');
    assert(item->data.literal.count == 1);

    segment = passgen_pattern_group_segment_get(&parser.pattern->group, 0);
    item = passgen_pattern_segment_get_item(segment, 1);
    assert(item);
    assert(item->kind == PASSGEN_PATTERN_GROUP);

    assert(1 == item->data.group.segments.len);

    segment = passgen_pattern_group_segment_get(&item->data.group, 0);
    assert(1 == segment->items.len);

    item = passgen_pattern_segment_get_item(segment, 0);
    assert(item);
    assert(item->kind == PASSGEN_PATTERN_LITERAL);
    assert(item->data.literal.codepoints[0] == 'b');
    assert(item->data.literal.count == 1);

    POSTAMBLE();

    return test_ok;
}

test_result test_parser_set_simple(void) {
    PREAMBLE();
    PARSE_CODEPOINT('[');
    PARSE_CODEPOINT('a');
    PARSE_CODEPOINT('b');
    PARSE_CODEPOINT(']');
    passgen_pattern_range *range;

    // single segment containing char 'a'
    assert(1 == parser.pattern->group.segments.len);

    segment = passgen_pattern_group_segment_get(&parser.pattern->group, 0);
    assert(segment);
    assert(1 == segment->items.len);

    item = passgen_pattern_segment_get_item(segment, 0);
    assert(item);
    assert(item->kind == PASSGEN_PATTERN_SET);

    assert(item->data.set.items.len == 2);

    range = passgen_pattern_set_range_get(&item->data.set, 0);
    assert(range);
    assert(range->start == 'a');
    assert(range->end == 'a');

    range = passgen_pattern_set_range_get(&item->data.set, 1);
    assert(range);
    assert(range->start == 'b');
    assert(range->end == 'b');

    POSTAMBLE();

    return test_ok;
}

test_result test_parser_set_simple_escaped(void) {
    PREAMBLE();
    PARSE_CODEPOINT('[');
    PARSE_CODEPOINT('a');
    PARSE_CODEPOINT_DOUBLE('\\', '-');
    PARSE_CODEPOINT('b');
    PARSE_CODEPOINT(']');
    passgen_pattern_range *range;

    // single segment containing char 'a'
    assert(1 == parser.pattern->group.segments.len);

    segment = passgen_pattern_group_segment_get(&parser.pattern->group, 0);
    assert(segment);
    assert(1 == segment->items.len);

    item = passgen_pattern_segment_get_item(segment, 0);
    assert(item);
    assert(item->kind == PASSGEN_PATTERN_SET);

    assert(item->data.set.items.len == 3);

    range = passgen_pattern_set_range_get(&item->data.set, 0);
    assert(range);
    assert(range->start == 'a');
    assert(range->end == 'a');

    range = passgen_pattern_set_range_get(&item->data.set, 1);
    assert(range);
    assert(range->start == '-');
    assert(range->end == '-');

    range = passgen_pattern_set_range_get(&item->data.set, 2);
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
    passgen_pattern_range *range;

    // single segment containing char 'a'
    assert(1 == parser.pattern->group.segments.len);

    segment = passgen_pattern_group_segment_get(&parser.pattern->group, 0);
    assert(segment);
    assert(1 == segment->items.len);

    item = passgen_pattern_segment_get_item(segment, 0);
    assert(item);
    assert(item->kind == PASSGEN_PATTERN_SET);

    assert(item->data.set.items.len == 1);

    range = passgen_pattern_set_range_get(&item->data.set, 0);
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
    passgen_pattern_range *range;

    // single segment containing char 'a'
    assert(1 == parser.pattern->group.segments.len);

    segment = passgen_pattern_group_segment_get(&parser.pattern->group, 0);
    assert(segment);
    assert(1 == segment->items.len);

    item = passgen_pattern_segment_get_item(segment, 0);
    assert(item);
    assert(item->kind == PASSGEN_PATTERN_SET);

    assert(item->data.set.items.len == 2);

    range = passgen_pattern_set_range_get(&item->data.set, 0);
    assert(range);
    assert(range->start == 'a');
    assert(range->end == 'b');

    range = passgen_pattern_set_range_get(&item->data.set, 1);
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
    assert(1 == parser.pattern->group.segments.len);

    segment = passgen_pattern_group_segment_get(&parser.pattern->group, 0);
    assert(segment);
    assert(1 == segment->items.len);

    item = passgen_pattern_segment_get_item(segment, 0);
    assert(item);
    assert(item->kind == PASSGEN_PATTERN_LITERAL);
    assert(item->data.literal.codepoints[0] == 'a');
    assert(item->data.literal.count == 1);
    assert(item->repeat.min == 2);
    assert(item->repeat.max == 2);

    passgen_parser_free(&parser);
    passgen_pattern_free(&parsed_pattern);

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
    assert(1 == parser.pattern->group.segments.len);

    segment = passgen_pattern_group_segment_get(&parser.pattern->group, 0);
    assert(segment);
    assert(1 == segment->items.len);

    item = passgen_pattern_segment_get_item(segment, 0);
    assert(item);
    assert(item->kind == PASSGEN_PATTERN_LITERAL);
    assert(item->data.literal.codepoints[0] == 'a');
    assert(item->data.literal.count == 1);
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

    assert(1 == parser.pattern->group.segments.len);

    segment = passgen_pattern_group_segment_get(&parser.pattern->group, 0);
    assert(segment);
    assert(1 == segment->items.len);

    item = passgen_pattern_segment_get_item(segment, 0);
    assert(item);
    assert(item->kind == PASSGEN_PATTERN_LITERAL);
    assert(item->data.literal.codepoints[0] == '(');
    assert(item->data.literal.codepoints[1] == '{');
    assert(item->data.literal.codepoints[2] == '[');
    assert(item->data.literal.codepoints[3] == '|');
    assert(item->data.literal.count == 4);
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
    PARSE_CODEPOINT('a');
    PARSE_CODEPOINT(')');
    PARSE_CODEPOINT('(');
    PARSE_CODEPOINT('b');
    PARSE_CODEPOINT(')');
    PARSE_CODEPOINT('?');

    // single segment containing char 'a'
    assert(1 == parser.pattern->group.segments.len);

    segment = passgen_pattern_group_segment_get(&parser.pattern->group, 0);
    assert(segment);
    assert(4 == segment->items.len);

    item = passgen_pattern_segment_get_item(segment, 0);
    assert(item);
    assert(item->kind == PASSGEN_PATTERN_LITERAL);
    assert(item->data.literal.codepoints[0] == 'a');
    assert(item->data.literal.count == 1);
    assert(item->repeat.min == 1);
    assert(item->repeat.max == 1);
    assert(item->maybe == false);

    item = passgen_pattern_segment_get_item(segment, 1);
    assert(item);
    assert(item->kind == PASSGEN_PATTERN_LITERAL);
    assert(item->data.literal.codepoints[0] == 'a');
    assert(item->data.literal.count == 1);
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
    PARSE_CODEPOINT_DOUBLE('\\', 'm');
    PARSE_CODEPOINT('{');
    PARSE_CODEPOINT('e');
    PARSE_CODEPOINT('n');
    PARSE_CODEPOINT('g');
    PARSE_CODEPOINT('l');
    PARSE_CODEPOINT('i');
    PARSE_CODEPOINT('s');
    PARSE_CODEPOINT('h');
    PARSE_CODEPOINT('}');

    assert(parser.state.len == 1);

    assert(1 == parser.pattern->group.segments.len);

    segment = passgen_pattern_group_segment_get(&parser.pattern->group, 0);
    assert(segment);
    assert(1 == segment->items.len);

    item = passgen_pattern_segment_get_item(segment, 0);
    assert(item);
    assert(item->kind == PASSGEN_PATTERN_SPECIAL);
    assert(item->repeat.min == 1);
    assert(item->repeat.max == 1);
    assert(item->maybe == false);
    assert(item->data.special.kind == PASSGEN_PATTERN_SPECIAL_MARKOV);

    POSTAMBLE();

    return test_ok;
}

test_result test_parser_mixed_special(void) {
    PREAMBLE();
    PARSE_CODEPOINT('a');
    PARSE_CODEPOINT_DOUBLE('\\', 'm');
    PARSE_CODEPOINT('{');
    PARSE_CODEPOINT('e');
    PARSE_CODEPOINT('n');
    PARSE_CODEPOINT('g');
    PARSE_CODEPOINT('l');
    PARSE_CODEPOINT('i');
    PARSE_CODEPOINT('s');
    PARSE_CODEPOINT('h');
    PARSE_CODEPOINT('}');

    assert(parser.state.len == 1);

    assert(1 == parser.pattern->group.segments.len);

    segment = passgen_pattern_group_segment_get(&parser.pattern->group, 0);
    assert(segment);
    assert(2 == segment->items.len);

    item = passgen_pattern_segment_get_item(segment, 0);
    assert(item);
    assert(item->kind == PASSGEN_PATTERN_LITERAL);
    assert(item->data.literal.codepoints[0] == 'a');
    assert(item->data.literal.count == 1);
    assert(item->repeat.min == 1);
    assert(item->repeat.max == 1);
    assert(item->maybe == false);

    item = passgen_pattern_segment_get_item(segment, 1);
    assert(item);
    assert(item->kind == PASSGEN_PATTERN_SPECIAL);
    assert(item->repeat.min == 1);
    assert(item->repeat.max == 1);
    assert(item->maybe == false);
    assert(item->data.special.kind == PASSGEN_PATTERN_SPECIAL_MARKOV);

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

    assert(1 == parser.pattern->group.segments.len);

    segment = passgen_pattern_group_segment_get(&parser.pattern->group, 0);
    assert(segment);
    assert(3 == segment->items.len);

    item = passgen_pattern_segment_get_item(segment, 0);
    assert(item);
    assert(item->kind == PASSGEN_PATTERN_LITERAL);
    assert(item->data.literal.count == 2);
    assert(item->data.literal.codepoints[0] == 'a');
    assert(item->data.literal.codepoints[1] == 'b');
    assert(item->data.literal.tainted == false);
    assert(item->repeat.min == 1);
    assert(item->repeat.max == 1);
    assert(item->maybe == false);

    item = passgen_pattern_segment_get_item(segment, 1);
    assert(item);
    assert(item->kind == PASSGEN_PATTERN_LITERAL);
    assert(item->data.literal.count == 1);
    assert(item->data.literal.codepoints[0] == 'c');
    assert(item->data.literal.tainted == true);
    assert(item->repeat.min == 1);
    assert(item->repeat.max == 1);
    assert(item->maybe == true);

    item = passgen_pattern_segment_get_item(segment, 2);
    assert(item);
    assert(item->kind == PASSGEN_PATTERN_LITERAL);
    assert(item->data.literal.count == 1);
    assert(item->data.literal.codepoints[0] == 'd');
    assert(item->data.literal.tainted == false);
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

    assert(1 == parser.pattern->group.segments.len);

    segment = passgen_pattern_group_segment_get(&parser.pattern->group, 0);
    assert(segment);
    assert(5 == segment->items.len);

    item = passgen_pattern_segment_get_item(segment, 0);
    assert(item);
    assert(item->kind == PASSGEN_PATTERN_LITERAL);
    assert(item->data.literal.count == 1);
    assert(item->data.literal.codepoints[0] == 'a');
    assert(item->data.literal.tainted == true);
    assert(item->repeat.min == 3);
    assert(item->repeat.max == 3);
    assert(item->maybe == false);

    item = passgen_pattern_segment_get_item(segment, 1);
    assert(item);
    assert(item->kind == PASSGEN_PATTERN_LITERAL);
    assert(item->data.literal.count == 7);
    assert(item->data.literal.codepoints[0] == 'b');
    assert(item->data.literal.codepoints[1] == 'c');
    assert(item->data.literal.codepoints[2] == 'd');
    assert(item->data.literal.codepoints[3] == 'e');
    assert(item->data.literal.codepoints[4] == 'f');
    assert(item->data.literal.codepoints[5] == 'g');
    assert(item->data.literal.codepoints[6] == 'h');
    assert(item->data.literal.tainted == false);
    assert(item->repeat.min == 1);
    assert(item->repeat.max == 1);
    assert(item->maybe == false);

    item = passgen_pattern_segment_get_item(segment, 2);
    assert(item);
    assert(item->kind == PASSGEN_PATTERN_LITERAL);
    assert(item->data.literal.count == 2);
    assert(item->data.literal.codepoints[0] == 'i');
    assert(item->data.literal.codepoints[1] == 'j');
    assert(item->data.literal.tainted == false);
    assert(item->repeat.min == 1);
    assert(item->repeat.max == 1);
    assert(item->maybe == false);

    item = passgen_pattern_segment_get_item(segment, 3);
    assert(item);
    assert(item->kind == PASSGEN_PATTERN_LITERAL);
    assert(item->data.literal.count == 1);
    assert(item->data.literal.codepoints[0] == 'k');
    assert(item->data.literal.tainted == true);
    assert(item->repeat.min == 2);
    assert(item->repeat.max == 2);
    assert(item->maybe == true);

    item = passgen_pattern_segment_get_item(segment, 4);
    assert(item);
    assert(item->kind == PASSGEN_PATTERN_LITERAL);
    assert(item->data.literal.count == 2);
    assert(item->data.literal.codepoints[0] == 'a');
    assert(item->data.literal.codepoints[1] == 'b');
    assert(item->data.literal.tainted == false);
    assert(item->repeat.min == 1);
    assert(item->repeat.max == 1);
    assert(item->maybe == false);

    POSTAMBLE();

    return test_ok;
}

#undef PREAMBLE
#undef POSTAMBLE
#undef PARSE_CODEPOINT
#undef PARSE_CODEPOINT_DOUBLE

/// Patterns that are known to be broken (result in a parse error).
const char *pattern_broken[] = {
    // closing groups that don't exist
    ")",
    ")))",
    "[a-z]))",
    // groups with end lower than start
    "[a-0]",
    "[z-a]",
    // groups with missing end
    "[a-",
    "[b-",
    "[b-]",
    "[za-",
    // unclosed groups
    "(",
    "(()",
    // unfinished escape sequence
    "\\",
    "\\u",
    "\\u{",
    "\\u{0a",
    // unicode literal payload too long
    "\\u{0000000000000",
    "\\u{123456789abcdef",
    // invalid utf8 sequences, taken from:
    // https://stackoverflow.com/questions/1301402/example-invalid-utf8-string
    // invalid 2-octet utf8
    "\xc3\x28",
    // invalid 3-octet utf8 (in second octet)
    "\xe2\x28\xa1",
    // invalid 3-octet utf8 (in third octet)
    "\xe2\x82\x28",
    // Invalid 4 Octet Sequence (in 2nd Octet)'
    "\xf0\x28\x8c\xbc",
    // Invalid 4 Octet Sequence (in 3rd Octet)
    "\xf0\x90\x28\xbc",
    // Invalid 4 Octet Sequence (in 4th Octet)
    "\xf0\x28\x8c\x28",
    // Valid 5 Octet Sequence (but not Unicode!)
    "\xf8\xa1\xa1\xa1\xa1",
    // Valid 6 Octet Sequence (but not Unicode!)
    "\xfc\xa1\xa1\xa1\xa1\xa1",
    NULL,
};

/// Make sure that the parser returns an error when parsing these known broken
/// patterns.
test_result test_parser_parse_broken(void) {
    for(int i = 0; pattern_broken[i]; i++) {
        passgen_pattern pattern;
        passgen_error error;
        int ret = passgen_parse(&pattern, &error, pattern_broken[i]);
        passgen_pattern_free(&pattern);
        assert(ret != 0);
    }

    return test_ok;
}

/// Patterns that are known to be working.
const char *pattern_working[] = {
    // character literal
    "",
    "a",
    "ab",
    "abc",
    "abcd",
    "abcde",
    "abcdef",
    "abcdefg",
    "abcdefgh",
    "abcdefghi",
    "abcdefghij",
    // character literal repetition
    "a{1}",
    "a{9}",
    "a{12}",
    "a{12,16}",
    // group
    "()",
    "(a)",
    "(a|b)",
    "(a|b|c)",
    "(a{2}|b|c)",
    "(a|b|c){2}",
    // character range
    "[a]",
    "[abc]",
    "[a-z]",
    "[a-z0-9]",
    "[a-z0-9!@#$%^&*]",
    "[a-z]{2}",
    "[a-z]{2,8}",
    // unicode literal
    "\\u{0a}",
    "\\u{0A}",
    "\\u{fc}",
    "\\u{FC}",
    "\\u{00fc}",
    "\\u{00FC}",
    "\\u{10ffff}",
    // special wordlist
    "\\w{english}",
    "\\m{english}",
    "\\p{pattern}",
    "\\w{englishenglish}",
    "\\w{englishenglishenglishenglish}",
    "\\w{englishenglishenglishenglishenglishenglishenglish}",
    "\\w{veryveryveryveryveryveryveryveryveryveryveryveryveryveryvery}",
    // very long pattern
    "abababababababababababababababababababababababababababababababababababab"
    "abababababababababababababababababababababababababababababababababababab"
    "abababababababababababababababababababababababababababababababababababab"
    "abababababababababababababababababababababababababababababababababababab",
    NULL,
};

/// Make sure that we can parse patterns that are known to be good.
test_result test_parser_parse_working(void) {
    for(int i = 0; pattern_working[i]; i++) {
        passgen_pattern pattern;
        passgen_error error;
        int ret = passgen_parse(&pattern, &error, pattern_working[i]);
        assert(ret == 0);
        passgen_pattern_free(&pattern);
    }

    return test_ok;
}

/// Make sure that we can parse random patterns. Some of these might be valid,
/// some might not. But none of these should be able to crash the parser in any
/// way.
test_result test_parser_parse_random_selected(void) {
    // How many random patterns to generate.
    size_t iterations = 10000;
    // Characters to choose from. Must be zero-terminated for `strlen` to work
    // on it.
    const char characters[] = "()[]|{},.abcdefghijklmnopqrstuvw0123456789\\";
    // Find out how many possible characters there are.
    size_t characters_len = strlen(characters);
    // Maximum length of the string to try parsing.
    size_t string_length = 16;
    // Storage for the string plus NULL-terminator.
    char string[string_length + 1];
    // Source of randomness.
    passgen_random *random = passgen_random_new(NULL);

    // Generate random strings and parse them.
    for(size_t i = 0; i < iterations; i++) {
        // Determine length of random string.
        size_t length = passgen_random_u8_max(random, string_length);
        // NUL-terminate the string.
        string[length] = 0;
        // Generate random characters.
        for(size_t c = 0; c < length; c++) {
            string[c] =
                characters[passgen_random_u8_max(random, characters_len)];
        }

        string[length] = 0;

        // Parse the string.
        passgen_pattern pattern;
        passgen_error error;
        passgen_parse(&pattern, &error, string);
        passgen_pattern_free(&pattern);
    }

    passgen_random_free(random);

    return test_ok;
}

/// Make sure that we can parse random patterns. Some of these might be valid,
/// some might not. But none of these should be able to crash the parser in any
/// way.
test_result test_parser_parse_random_ascii_printable(void) {
    size_t iterations = 10000;
    size_t string_length = 16;
    char string[string_length + 1];
    passgen_random *random = passgen_random_new(NULL);

    for(size_t i = 0; i < iterations; i++) {
        for(size_t c = 0; c < string_length; c++) {
            string[c] = 33 + passgen_random_u8_max(random, 93);
        }
        string[string_length] = 0;

        // Parse the string.
        passgen_pattern pattern;
        passgen_error error;
        passgen_parse(&pattern, &error, string);
        passgen_pattern_free(&pattern);
    }

    passgen_random_free(random);

    return test_ok;
}

test_result test_parser_parse_random_unicode(void) {
    size_t iterations = 10000;
    size_t string_length = 16;
    uint32_t string[string_length];
    passgen_random *random = passgen_random_new(NULL);

    for(size_t i = 0; i < iterations; i++) {
        for(size_t c = 0; c < string_length; c++) {
            string[c] = 33 + passgen_random_u32_max(random, PASSGEN_UNICODE_MAX);
        }

        // Parse the string.
        passgen_parser parser;
        passgen_pattern pattern;
        passgen_parser_init(&parser, &pattern);
        passgen_parser_unicode(&parser, string, string_length);
        passgen_pattern_free(parser.pattern);
        passgen_parser_free(&parser);
    }

    passgen_random_free(random);

    return test_ok;
}
