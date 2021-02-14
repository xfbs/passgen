#include <criterion/criterion.h>
#include <passgen/parser.h>
#include <passgen/data/parser.h>
#include <passgen/data/pattern_kind.h>

// Parse a variable number of codepoints, and make sure that they all show up
// as chars. This doesn't work with anything that is escaped.
static void parse_codepoints(size_t length, uint32_t codepoints[]) {
    struct passgen_parser parser;
    passgen_parser_init(&parser);
    passgen_parser_unicode(&parser, &codepoints[0], length);
    cr_assert(passgen_parse_finish(&parser) == 0);

    cr_assert_eq(parser.pattern.group.segments.len, 1);

    struct passgen_pattern_segment *segment;
    segment = passgen_pattern_group_get_segment(&parser.pattern.group, 0);
    cr_assert(segment);
    cr_assert_eq(segment->items.len, 1);

    size_t item_num = 0;
    size_t char_num = 0;
    for(size_t pos = 0; pos < length; pos++) {
        cr_assert(item_num < segment->items.len);

        struct passgen_pattern_item *item;
        item = passgen_pattern_segment_get_item(segment, item_num);
        cr_assert_eq(item->kind, PASSGEN_PATTERN_CHAR);
        cr_assert_eq(item->data.chars.codepoints[char_num], codepoints[pos]);

        // move on to next item or char
        char_num++;
        if(char_num == item->data.chars.count) {
            char_num = 0;
            item_num++;
        }
    }

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

Test(parser, codepoints) {
    parse_codepoints(1, (uint32_t []){'a'});
    parse_codepoints(1, (uint32_t []){'!'});
    parse_codepoints(1, (uint32_t []){'-'});
    parse_codepoints(1, (uint32_t []){','});
    parse_codepoints(1, (uint32_t []){0xFF});
    parse_codepoints(1, (uint32_t []){0x1F642});
    parse_codepoints(2, (uint32_t[]){'a', 'b'});
    parse_codepoints(2, (uint32_t[]){'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p'});
    parse_codepoints(2, (uint32_t[]){'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'});
}
