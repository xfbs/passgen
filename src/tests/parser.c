#include "passgen/parser/parser.h"

#include "passgen/pattern/chars.h"
#include "passgen/pattern/group.h"
#include "passgen/pattern/parser.h"
#include "passgen/pattern/pattern.h"
#include "passgen/pattern/pattern_kind.h"
#include "passgen/pattern/range.h"
#include "passgen/pattern/repeat.h"
#include "passgen/pattern/segment.h"
#include "passgen/pattern/segment_item.h"
#include "passgen/pattern/set.h"
#include "passgen/pattern/token.h"
#include "passgen/util/random.h"
#include "passgen/util/stack.h"
#include "passgen/parser/token.h"
#include "tests.h"
#include <passgen/passgen.h>

#define PREAMBLE()                                  \
    struct passgen_parser parser;                   \
    struct passgen_token_parser token_parser = {0}; \
    struct passgen_token token = {0};               \
    struct passgen_pattern_segment *segment;        \
    struct passgen_pattern_item *item;              \
    passgen_parser_init(&parser);

#define POSTAMBLE()                              \
    assert_eq(0, passgen_parse_finish(&parser)); \
    passgen_parser_free(&parser)

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
    struct passgen_parser parser;
    struct passgen_pattern_segment *segment;
    passgen_parser_init(&parser);

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
    assert(item->data.chars.codepoints[0] == 'a');
    assert(item->data.chars.count == 1);

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
    assert(item->data.chars.codepoints[0] == 'a');
    assert(item->data.chars.codepoints[1] == 'b');
    assert(item->data.chars.count == 2);

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
    assert(item->data.chars.codepoints[0] == 'a');
    assert(item->data.chars.count == 1);

    segment = passgen_pattern_group_get_segment(&parser.pattern.group, 1);
    assert(1 == segment->items.len);

    item = passgen_pattern_segment_get_item(segment, 0);
    assert(item);
    assert(item->kind == PASSGEN_PATTERN_CHAR);
    assert(item->data.chars.codepoints[0] == 'b');
    assert(item->data.chars.count == 1);

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
    assert(item->data.chars.codepoints[0] == 'a');
    assert(item->data.chars.count == 1);
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
    assert(item->data.chars.codepoints[0] == 'a');
    assert(item->data.chars.count == 1);

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
    assert(item->data.chars.codepoints[0] == 'b');
    assert(item->data.chars.count == 1);

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
    assert(item->data.chars.codepoints[0] == 'a');
    assert(item->data.chars.count == 1);
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
    assert(item->data.chars.codepoints[0] == 'a');
    assert(item->data.chars.count == 1);
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
    assert(item->data.chars.codepoints[0] == '(');
    assert(item->data.chars.codepoints[1] == '{');
    assert(item->data.chars.codepoints[2] == '[');
    assert(item->data.chars.codepoints[3] == '|');
    assert(item->data.chars.count == 4);
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
    assert(item->data.chars.codepoints[0] == 'a');
    assert(item->data.chars.count == 1);
    assert(item->repeat.min == 1);
    assert(item->repeat.max == 1);
    assert(item->maybe == false);

    item = passgen_pattern_segment_get_item(segment, 1);
    assert(item);
    assert(item->kind == PASSGEN_PATTERN_CHAR);
    assert(item->data.chars.codepoints[0] == 'a');
    assert(item->data.chars.count == 1);
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

    assert(1 == parser.pattern.group.segments.len);

    segment = passgen_pattern_group_get_segment(&parser.pattern.group, 0);
    assert(segment);
    assert(2 == segment->items.len);

    item = passgen_pattern_segment_get_item(segment, 0);
    assert(item);
    assert(item->kind == PASSGEN_PATTERN_CHAR);
    assert(item->data.chars.codepoints[0] == 'a');
    assert(item->data.chars.count == 1);
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

    assert(1 == parser.pattern.group.segments.len);

    segment = passgen_pattern_group_get_segment(&parser.pattern.group, 0);
    assert(segment);
    assert(3 == segment->items.len);

    item = passgen_pattern_segment_get_item(segment, 0);
    assert(item);
    assert(item->kind == PASSGEN_PATTERN_CHAR);
    assert(item->data.chars.count == 2);
    assert(item->data.chars.codepoints[0] == 'a');
    assert(item->data.chars.codepoints[1] == 'b');
    assert(item->data.chars.tainted == false);
    assert(item->repeat.min == 1);
    assert(item->repeat.max == 1);
    assert(item->maybe == false);

    item = passgen_pattern_segment_get_item(segment, 1);
    assert(item);
    assert(item->kind == PASSGEN_PATTERN_CHAR);
    assert(item->data.chars.count == 1);
    assert(item->data.chars.codepoints[0] == 'c');
    assert(item->data.chars.tainted == true);
    assert(item->repeat.min == 1);
    assert(item->repeat.max == 1);
    assert(item->maybe == true);

    item = passgen_pattern_segment_get_item(segment, 2);
    assert(item);
    assert(item->kind == PASSGEN_PATTERN_CHAR);
    assert(item->data.chars.count == 1);
    assert(item->data.chars.codepoints[0] == 'd');
    assert(item->data.chars.tainted == false);
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
    assert(item->data.chars.count == 1);
    assert(item->data.chars.codepoints[0] == 'a');
    assert(item->data.chars.tainted == true);
    assert(item->repeat.min == 3);
    assert(item->repeat.max == 3);
    assert(item->maybe == false);

    item = passgen_pattern_segment_get_item(segment, 1);
    assert(item);
    assert(item->kind == PASSGEN_PATTERN_CHAR);
    assert(item->data.chars.count == 7);
    assert(item->data.chars.codepoints[0] == 'b');
    assert(item->data.chars.codepoints[1] == 'c');
    assert(item->data.chars.codepoints[2] == 'd');
    assert(item->data.chars.codepoints[3] == 'e');
    assert(item->data.chars.codepoints[4] == 'f');
    assert(item->data.chars.codepoints[5] == 'g');
    assert(item->data.chars.codepoints[6] == 'h');
    assert(item->data.chars.tainted == false);
    assert(item->repeat.min == 1);
    assert(item->repeat.max == 1);
    assert(item->maybe == false);

    item = passgen_pattern_segment_get_item(segment, 2);
    assert(item);
    assert(item->kind == PASSGEN_PATTERN_CHAR);
    assert(item->data.chars.count == 2);
    assert(item->data.chars.codepoints[0] == 'i');
    assert(item->data.chars.codepoints[1] == 'j');
    assert(item->data.chars.tainted == false);
    assert(item->repeat.min == 1);
    assert(item->repeat.max == 1);
    assert(item->maybe == false);

    item = passgen_pattern_segment_get_item(segment, 3);
    assert(item);
    assert(item->kind == PASSGEN_PATTERN_CHAR);
    assert(item->data.chars.count == 1);
    assert(item->data.chars.codepoints[0] == 'k');
    assert(item->data.chars.tainted == true);
    assert(item->repeat.min == 2);
    assert(item->repeat.max == 2);
    assert(item->maybe == true);

    item = passgen_pattern_segment_get_item(segment, 4);
    assert(item);
    assert(item->kind == PASSGEN_PATTERN_CHAR);
    assert(item->data.chars.count == 2);
    assert(item->data.chars.codepoints[0] == 'a');
    assert(item->data.chars.codepoints[1] == 'b');
    assert(item->data.chars.tainted == false);
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
    // unicode literal payload wrong chars

    // repeat without anything before
    "{0}",
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
test_result test_parser_can_parse_broken(void) {
    for(int i = 0; pattern_broken[i]; i++) {
        struct passgen_pattern pattern;
        passgen_error error;
        int ret = passgen_parse(&pattern, &error, pattern_broken[i]);
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
    NULL,
};

/// Make sure that we can parse patterns that are known to be good.
test_result test_parser_can_parse_working(void) {
    for(int i = 0; pattern_working[i]; i++) {
        struct passgen_pattern pattern;
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
test_result test_parser_can_parse_random(void) {
    // How many random patterns to generate.
    size_t iterations = 10000;
    // Characters to choose from. Must be zero-terminated for `strlen` to work
    // on it.
    const char characters[] =
        {'(', ')', '[', ']', '|', '{', '}', ',', 'a', 'z', '0', '9', '\\', 0};
    // Find out how many possible characters there are.
    size_t characters_len = strlen(characters);
    // Maximum length of the string to try parsing.
    size_t string_length = 16;
    // Storage for the string plus NULL-terminator.
    char string[string_length + 1];
    // Source of randomness.
    passgen_random *random = passgen_random_new();

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

        // Parse the string.
        struct passgen_pattern pattern;
        passgen_error error;
        int ret = passgen_parse(&pattern, &error, string);
        if(ret == 0) {
            passgen_pattern_free(&pattern);
        }
    }

    passgen_random_free(random);

    return test_ok;
}
