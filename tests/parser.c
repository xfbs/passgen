#include "passgen/parser.h"
#include "passgen/pattern/item.h"
#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include "tests.h"

test_result test_parser_empty(void) {
    struct parser parser = {0};
    struct passgen_token_parser token_parser = {0};
    struct passgen_token token = {0};
    struct passgen_pattern_segment *segment;

    assert(0 == parser_init(&parser));

    // single empty segment
    assert(1 == parser.pattern.group.segments.len);
    segment = passgen_pattern_group_get_segment(&parser.pattern.group, 0);
    assert(0 == segment->items.len);

    return test_ok;
}

test_result test_parser_single_char(void) {
    struct parser parser = {0};
    struct passgen_token_parser token_parser = {0};
    struct passgen_token token = {0};
    struct parser_state *state;
    struct passgen_pattern_segment *segment;
    struct passgen_pattern_item *item;

    assert(0 == parser_init(&parser));
    assert(passgen_token_parse(&token_parser, &token, 'a') == PASSGEN_TOKEN_INIT);
    assert(0 == parse_token(&parser, &token));

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
    struct parser parser = {0};
    struct passgen_token_parser token_parser = {0};
    struct passgen_token token = {0};
    struct passgen_pattern_segment *segment;
    struct passgen_pattern_item *item;

    assert(0 == parser_init(&parser));
    assert(passgen_token_parse(&token_parser, &token, 'a') == PASSGEN_TOKEN_INIT);
    assert(0 == parse_token(&parser, &token));
    assert(passgen_token_parse(&token_parser, &token, 'b') == PASSGEN_TOKEN_INIT);
    assert(0 == parse_token(&parser, &token));

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
    struct parser parser = {0};
    struct passgen_token_parser token_parser = {0};
    struct passgen_token token = {0};
    passgen_pattern_segment_t *segment;

    assert(0 == parser_init(&parser));
    assert(passgen_token_parse(&token_parser, &token, 'a') == PASSGEN_TOKEN_INIT);
    assert(0 == parse_token(&parser, &token));
    assert(passgen_token_parse(&token_parser, &token, '|') == PASSGEN_TOKEN_INIT);
    assert(0 == parse_token(&parser, &token));
    assert(passgen_token_parse(&token_parser, &token, 'b') == PASSGEN_TOKEN_INIT);
    assert(0 == parse_token(&parser, &token));

    assert(2 == parser.pattern.group.segments.len);

    segment = passgen_pattern_group_get_segment(&parser.pattern.group, 0);
    assert(1 == segment->items.len);

    segment = passgen_pattern_group_get_segment(&parser.pattern.group, 1);
    assert(1 == segment->items.len);

    return test_ok;
}

