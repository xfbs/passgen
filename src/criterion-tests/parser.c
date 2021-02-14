#include <criterion/criterion.h>
#include <passgen/parser.h>
#include <passgen/data/parser.h>
#include <passgen/data/pattern_kind.h>

static void parse_codepoint(uint32_t codepoint) {
    struct passgen_parser parser;
    passgen_parser_init(&parser);
    passgen_parser_unicode(&parser, &codepoint, 1);
    cr_assert(passgen_parse_finish(&parser) == 0);

    cr_assert_eq(parser.pattern.group.segments.len, 1);

    struct passgen_pattern_segment *segment;
    segment = passgen_pattern_group_get_segment(&parser.pattern.group, 0);
    cr_assert(segment);
    cr_assert_eq(segment->items.len, 1);

    struct passgen_pattern_item *item;
    item = passgen_pattern_segment_get_item(segment, 0);

    cr_assert_eq(item->kind, PASSGEN_PATTERN_CHAR);
    cr_assert_eq(item->data.chars.codepoints[0], codepoint);
    cr_assert_eq(item->data.chars.count, 1);

    passgen_parser_free(&parser);
}

Test(parser, empty) {
    struct passgen_parser parser;
    struct passgen_pattern_segment *segment;
    passgen_parser_init(&parser);
    cr_assert(passgen_parse_finish(&parser) == 0);

    // single empty segment
    cr_assert(1 == parser.pattern.group.segments.len);
    segment = passgen_pattern_group_get_segment(&parser.pattern.group, 0);
    cr_assert(0 == segment->items.len);

    passgen_parser_free(&parser);
}

Test(parser, codepoint) {
    parse_codepoint('a');
    parse_codepoint('!');
    parse_codepoint('-');
    parse_codepoint(',');
    parse_codepoint(0xFF);
    parse_codepoint(0x1F642);
}
