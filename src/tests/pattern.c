#include "passgen/pattern.h"

#include "passgen/memory.h"
#include "tests.h"

test_result test_pattern_parse_str() {
    passgen_mem_accounting_t acc = passgen_mem_accounting_new(NULL);
    passgen_mem_t mem = passgen_mem_accounting(&acc);

    pattern_t pattern;
    pattern_result_t result;
    const char *str;
    pattern_segments_t *segments;
    pattern_segment_t *segment;

    str = "abc";
    result = pattern_parse(&pattern, str, 0, &mem);
    assert(result.ok);
    assert(pattern.mem == &mem);
    assert(pattern.pattern == str);
    assert(pattern.group.pos.offset == 0);
    assert(pattern.group.pos.length == 3);
    assert(pattern.group.segments.len == 1);
    segments = passgen_array_get(
        &pattern.group.segments,
        sizeof(pattern_segments_t),
        0);
    assert(segments);
    assert(segments->pos.offset == 0);
    assert(segments->pos.length == 3);
    assert(segments->items.len == 3);
    segment = passgen_array_get(&segments->items, sizeof(pattern_segment_t), 0);
    assert(segment);
    assert(segment->kind == PATTERN_CHAR);
    assert(segment->data.character.pos.offset == 0);
    assert(segment->data.character.pos.length == 1);
    assert(segment->data.character.codepoint == 'a');
    segment = passgen_array_get(&segments->items, sizeof(pattern_segment_t), 1);
    assert(segment);
    assert(segment->kind == PATTERN_CHAR);
    assert(segment->data.character.pos.offset == 1);
    assert(segment->data.character.pos.length == 1);
    assert(segment->data.character.codepoint == 'b');
    segment = passgen_array_get(&segments->items, sizeof(pattern_segment_t), 2);
    assert(segment);
    assert(segment->kind == PATTERN_CHAR);
    assert(segment->data.character.pos.offset == 2);
    assert(segment->data.character.pos.length == 1);
    assert(segment->data.character.codepoint == 'c');

    // free, make sure no memory leaks
    pattern_free(&pattern);
    assert(passgen_mem_accounting_check(&acc));
    passgen_mem_accounting_cleanup(&acc);

    return test_ok;
}

test_result test_pattern_parse_group() {
    passgen_mem_accounting_t acc = passgen_mem_accounting_new(NULL);
    passgen_mem_t mem = passgen_mem_accounting(&acc);

    pattern_t pattern;
    pattern_result_t result;
    const char *str;
    pattern_segments_t *segments;
    pattern_segment_t *segment;

    str = "(abc)";
    result = pattern_parse(&pattern, str, 1, &mem);
    assert(result.ok);
    assert(pattern.mem == &mem);
    assert(pattern.pattern == str);
    assert(pattern.group.pos.offset == 0);
    assert(pattern.group.pos.length == 5);
    assert(pattern.group.segments.len == 1);
    segments = passgen_array_get(
        &pattern.group.segments,
        sizeof(pattern_segments_t),
        0);
    assert(segments);
    assert(segments->pos.offset == 0);
    assert(segments->pos.length == 5);
    assert(segments->items.len == 1);
    segment = passgen_array_get(&segments->items, sizeof(pattern_segment_t), 0);
    assert(segment);
    assert(segment->kind == PATTERN_GROUP);
    assert(segment->data.group.pos.offset == 1);
    assert(segment->data.group.pos.length == 3);
    segments = passgen_array_get(
        &segment->data.group.segments,
        sizeof(pattern_segments_t),
        0);
    assert(segments);
    assert(segments->items.len == 3);

    // free, make sure no memory leaks
    pattern_free(&pattern);
    assert(passgen_mem_accounting_check(&acc));
    passgen_mem_accounting_cleanup(&acc);

    return test_ok;
}

test_result test_pattern_parse_strrep() {
    passgen_mem_accounting_t acc = passgen_mem_accounting_new(NULL);
    passgen_mem_t mem = passgen_mem_accounting(&acc);

    pattern_t pattern;
    pattern_result_t result;
    const char *str;
    pattern_segments_t *segments;
    pattern_segment_t *segment;

    str = "ab{11}c{14,19}";
    result = pattern_parse(&pattern, str, 0, &mem);
    assert(result.ok);
    assert(pattern.mem == &mem);
    assert(pattern.pattern == str);
    assert(pattern.group.pos.offset == 0);
    assert(pattern.group.pos.length == 14);
    assert(pattern.group.segments.len == 1);
    segments = passgen_array_get(
        &pattern.group.segments,
        sizeof(pattern_segments_t),
        0);
    assert(segments);
    assert(segments->pos.offset == 0);
    assert(segments->pos.length == 14);
    assert(segments->items.len == 3);
    segment = passgen_array_get(&segments->items, sizeof(pattern_segment_t), 0);
    assert(segment->kind == PATTERN_CHAR);
    assert(segment->data.character.pos.offset == 0);
    assert(segment->data.character.pos.length == 1);
    assert(segment->data.character.codepoint == 'a');
    assert(segment->data.character.repeat.min == 1);
    assert(segment->data.character.repeat.max == 1);
    segment = passgen_array_get(&segments->items, sizeof(pattern_segment_t), 1);
    assert(segment->kind == PATTERN_CHAR);
    assert(segment->data.character.pos.offset == 1);
    assert(segment->data.character.pos.length == 5);
    assert(segment->data.character.codepoint == 'b');
    assert(segment->data.character.repeat.min == 11);
    assert(segment->data.character.repeat.max == 11);
    segment = passgen_array_get(&segments->items, sizeof(pattern_segment_t), 2);
    assert(segment->kind == PATTERN_CHAR);
    assert(segment->data.character.pos.offset == 6);
    assert(segment->data.character.pos.length == 8);
    assert(segment->data.character.codepoint == 'c');
    assert(segment->data.character.repeat.min == 14);
    assert(segment->data.character.repeat.max == 19);

    // free, make sure no memory leaks
    pattern_free(&pattern);
    assert(passgen_mem_accounting_check(&acc));
    passgen_mem_accounting_cleanup(&acc);

    return test_ok;
}

test_result test_pattern_parse_rangerep() {
    passgen_mem_accounting_t acc = passgen_mem_accounting_new(NULL);
    passgen_mem_t mem = passgen_mem_accounting(&acc);

    pattern_t pattern;
    pattern_result_t result;
    const char *str;
    pattern_segments_t *segments;
    pattern_segment_t *segment;

    str = "[n][a]{5}[b-d]{8,127}";
    result = pattern_parse(&pattern, str, 0, &mem);
    assert(result.ok);
    assert(pattern.mem == &mem);
    assert(pattern.pattern == str);
    assert(pattern.group.pos.offset == 0);
    assert(pattern.group.pos.length == 21);
    assert(pattern.group.segments.len == 1);
    segments = passgen_array_get(
        &pattern.group.segments,
        sizeof(pattern_segments_t),
        0);
    assert(segments);
    assert(segments->pos.offset == 0);
    assert(segments->pos.length == 21);
    assert(segments->items.len == 3);
    segment = passgen_array_get(&segments->items, sizeof(pattern_segment_t), 0);
    assert(segment->kind == PATTERN_RANGE);
    assert(segment->data.range.pos.offset == 0);
    assert(segment->data.range.pos.length == 3);
    assert(segment->data.range.repeat.min == 1);
    assert(segment->data.range.repeat.max == 1);
    segment = passgen_array_get(&segments->items, sizeof(pattern_segment_t), 1);
    assert(segment->kind == PATTERN_RANGE);
    assert(segment->data.range.pos.offset == 3);
    assert(segment->data.range.pos.length == 6);
    assert(segment->data.range.repeat.min == 5);
    assert(segment->data.range.repeat.max == 5);
    segment = passgen_array_get(&segments->items, sizeof(pattern_segment_t), 2);
    assert(segment->kind == PATTERN_RANGE);
    assert(segment->data.range.pos.offset == 9);
    assert(segment->data.range.pos.length == 12);
    assert(segment->data.range.repeat.min == 8);
    assert(segment->data.range.repeat.max == 127);

    // free, make sure no memory leaks
    pattern_free(&pattern);
    assert(passgen_mem_accounting_check(&acc));
    passgen_mem_accounting_cleanup(&acc);

    return test_ok;
}

test_result test_pattern_parse_orstr() {
    passgen_mem_accounting_t acc = passgen_mem_accounting_new(NULL);
    passgen_mem_t mem = passgen_mem_accounting(&acc);

    pattern_t pattern;
    pattern_result_t result;
    const char *str;
    pattern_segments_t *segments;
    pattern_segment_t *segment;

    str = "abc|def";
    result = pattern_parse(&pattern, str, 0, &mem);
    assert(result.ok);
    assert(pattern.pattern == str);
    assert(pattern.mem == &mem);
    assert(pattern.group.pos.offset == 0);
    assert(pattern.group.pos.length == 7);
    assert(pattern.group.segments.len == 2);
    segments = passgen_array_get(
        &pattern.group.segments,
        sizeof(pattern_segments_t),
        0);
    assert(segments);
    assert(segments->pos.offset == 0);
    assert(segments->pos.length == 3);
    assert(segments->items.len == 3);
    segment = passgen_array_get(&segments->items, sizeof(pattern_segment_t), 0);
    assert(segment);
    assert(segment->kind == PATTERN_CHAR);
    assert(segment->data.character.pos.offset == 0);
    assert(segment->data.character.pos.length == 1);
    assert(segment->data.character.codepoint == 'a');
    segment = passgen_array_get(&segments->items, sizeof(pattern_segment_t), 1);
    assert(segment);
    assert(segment->kind == PATTERN_CHAR);
    assert(segment->data.character.pos.offset == 1);
    assert(segment->data.character.pos.length == 1);
    assert(segment->data.character.codepoint == 'b');
    segment = passgen_array_get(&segments->items, sizeof(pattern_segment_t), 2);
    assert(segment);
    assert(segment->kind == PATTERN_CHAR);
    assert(segment->data.character.pos.offset == 2);
    assert(segment->data.character.pos.length == 1);
    assert(segment->data.character.codepoint == 'c');
    segments = passgen_array_get(
        &pattern.group.segments,
        sizeof(pattern_segments_t),
        1);
    assert(segments);
    assert(segments->pos.offset == 4);
    assert(segments->pos.length == 3);
    assert(segments->items.len == 3);
    segment = passgen_array_get(&segments->items, sizeof(pattern_segment_t), 0);
    assert(segment);
    assert(segment->kind == PATTERN_CHAR);
    assert(segment->data.character.pos.offset == 4);
    assert(segment->data.character.pos.length == 1);
    assert(segment->data.character.codepoint == 'd');
    segment = passgen_array_get(&segments->items, sizeof(pattern_segment_t), 1);
    assert(segment);
    assert(segment->kind == PATTERN_CHAR);
    assert(segment->data.character.pos.offset == 5);
    assert(segment->data.character.pos.length == 1);
    assert(segment->data.character.codepoint == 'e');
    segment = passgen_array_get(&segments->items, sizeof(pattern_segment_t), 2);
    assert(segment);
    assert(segment->kind == PATTERN_CHAR);
    assert(segment->data.character.pos.offset == 6);
    assert(segment->data.character.pos.length == 1);
    assert(segment->data.character.codepoint == 'f');

    // free, make sure no memory leaks
    pattern_free(&pattern);
    assert(passgen_mem_accounting_check(&acc));
    passgen_mem_accounting_cleanup(&acc);

    return test_ok;
}

test_result test_pattern_parse_range() {
    passgen_mem_accounting_t acc = passgen_mem_accounting_new(NULL);
    passgen_mem_t mem = passgen_mem_accounting(&acc);

    pattern_t pattern;
    pattern_result_t result;
    const char *str;
    pattern_segments_t *segments;
    pattern_segment_t *segment;
    pattern_ranges_t *ranges;
    pattern_range_t *range;

    str = "[abc-d]";
    result = pattern_parse(&pattern, str, 0, &mem);
    assert(result.ok);
    assert(pattern.pattern == str);
    assert(pattern.mem == &mem);
    assert(pattern.group.pos.offset == 0);
    assert(pattern.group.pos.length == 7);
    assert(pattern.group.segments.len == 1);
    segments = passgen_array_get(
        &pattern.group.segments,
        sizeof(pattern_segments_t),
        0);
    assert(segments);
    assert(segments->pos.offset == 0);
    assert(segments->pos.length == 7);
    assert(segments->items.len == 1);
    segment = passgen_array_get(&segments->items, sizeof(pattern_segment_t), 0);
    assert(segment);
    assert(segment->kind == PATTERN_RANGE);
    ranges = &segment->data.range;
    assert(ranges->pos.offset == 0);
    assert(ranges->pos.length == 7);
    assert(ranges->items.len == 3);
    range = passgen_array_get(&ranges->items, sizeof(pattern_range_t), 0);
    assert(range->start == 'a');
    assert(range->end == 'a');
    assert(range->pos.offset == 1);
    assert(range->pos.length == 1);
    range = passgen_array_get(&ranges->items, sizeof(pattern_range_t), 1);
    assert(range->start == 'b');
    assert(range->end == 'b');
    assert(range->pos.offset == 2);
    assert(range->pos.length == 1);
    range = passgen_array_get(&ranges->items, sizeof(pattern_range_t), 2);
    assert(range->start == 'c');
    assert(range->end == 'd');
    assert(range->pos.offset == 3);
    assert(range->pos.length == 3);

    // free, make sure no memory leaks
    pattern_free(&pattern);
    assert(passgen_mem_accounting_check(&acc));
    passgen_mem_accounting_cleanup(&acc);

    return test_ok;
}

test_result test_pattern_escapable() {
    passgen_mem_accounting_t acc = passgen_mem_accounting_new(NULL);
    passgen_mem_t mem = passgen_mem_accounting(&acc);

    pattern_t pattern;
    pattern_result_t result;
    const char *str;
    pattern_segments_t *segments;
    pattern_segment_t *segment;

    str = "\\[\\]\\(\\)\\{\\}\\|";
    result = pattern_parse(&pattern, str, 0, &mem);
    assert(result.ok);
    assert(pattern.mem == &mem);
    assert(pattern.pattern == str);
    assert(pattern.group.pos.offset == 0);
    assert(pattern.group.pos.length == 14);
    assert(pattern.group.segments.len == 1);
    segments = passgen_array_get(
        &pattern.group.segments,
        sizeof(pattern_segments_t),
        0);
    assert(segments);
    assert(segments->pos.offset == 0);
    assert(segments->pos.length == 14);
    assert(segments->items.len == 7);
    segment = passgen_array_get(&segments->items, sizeof(pattern_segment_t), 0);
    assert(segment);
    assert(segment->kind == PATTERN_CHAR);
    assert(segment->data.character.pos.offset == 0);
    assert(segment->data.character.pos.length == 2);
    assert(segment->data.character.codepoint == '[');
    segment = passgen_array_get(&segments->items, sizeof(pattern_segment_t), 1);
    assert(segment);
    assert(segment->kind == PATTERN_CHAR);
    assert(segment->data.character.pos.offset == 2);
    assert(segment->data.character.pos.length == 2);
    assert(segment->data.character.codepoint == ']');
    segment = passgen_array_get(&segments->items, sizeof(pattern_segment_t), 2);
    assert(segment);
    assert(segment->kind == PATTERN_CHAR);
    assert(segment->data.character.pos.offset == 4);
    assert(segment->data.character.pos.length == 2);
    assert(segment->data.character.codepoint == '(');
    segment = passgen_array_get(&segments->items, sizeof(pattern_segment_t), 3);
    assert(segment);
    assert(segment->kind == PATTERN_CHAR);
    assert(segment->data.character.pos.offset == 6);
    assert(segment->data.character.pos.length == 2);
    assert(segment->data.character.codepoint == ')');
    segment = passgen_array_get(&segments->items, sizeof(pattern_segment_t), 4);
    assert(segment);
    assert(segment->kind == PATTERN_CHAR);
    assert(segment->data.character.pos.offset == 8);
    assert(segment->data.character.pos.length == 2);
    assert(segment->data.character.codepoint == '{');
    segment = passgen_array_get(&segments->items, sizeof(pattern_segment_t), 5);
    assert(segment);
    assert(segment->kind == PATTERN_CHAR);
    assert(segment->data.character.pos.offset == 10);
    assert(segment->data.character.pos.length == 2);
    assert(segment->data.character.codepoint == '}');
    segment = passgen_array_get(&segments->items, sizeof(pattern_segment_t), 6);
    assert(segment);
    assert(segment->kind == PATTERN_CHAR);
    assert(segment->data.character.pos.offset == 12);
    assert(segment->data.character.pos.length == 2);
    assert(segment->data.character.codepoint == '|');

    // free, make sure no memory leaks
    pattern_free(&pattern);
    assert(passgen_mem_accounting_check(&acc));
    passgen_mem_accounting_cleanup(&acc);

    return test_ok;
}

test_result test_pattern_utf8() {
    passgen_mem_accounting_t acc = passgen_mem_accounting_new(NULL);
    passgen_mem_t mem = passgen_mem_accounting(&acc);

    pattern_t pattern;
    pattern_result_t result;
    const char *str;
    pattern_segments_t *segments;
    pattern_segment_t *segment;

    str = "üöäßç";
    result = pattern_parse(&pattern, str, 0, &mem);
    assert(result.ok);
    segments = passgen_array_get(
        &pattern.group.segments,
        sizeof(pattern_segments_t),
        0);
    assert(segments);
    segment = passgen_array_get(&segments->items, sizeof(pattern_segment_t), 0);
    assert(segment);
    assert(segment->kind == PATTERN_CHAR);
    assert(segment->data.character.codepoint == 0xFC);
    segment = passgen_array_get(&segments->items, sizeof(pattern_segment_t), 1);
    assert(segment);
    assert(segment->kind == PATTERN_CHAR);
    assert(segment->data.character.codepoint == 0xF6);
    segment = passgen_array_get(&segments->items, sizeof(pattern_segment_t), 2);
    assert(segment);
    assert(segment->kind == PATTERN_CHAR);
    assert(segment->data.character.codepoint == 0xE4);
    segment = passgen_array_get(&segments->items, sizeof(pattern_segment_t), 3);
    assert(segment);
    assert(segment->kind == PATTERN_CHAR);
    assert(segment->data.character.codepoint == 0xDF);
    segment = passgen_array_get(&segments->items, sizeof(pattern_segment_t), 4);
    assert(segment);
    assert(segment->kind == PATTERN_CHAR);
    assert(segment->data.character.codepoint == 0xE7);

    // free, make sure no memory leaks
    pattern_free(&pattern);
    assert(passgen_mem_accounting_check(&acc));
    passgen_mem_accounting_cleanup(&acc);

    return test_ok;
}

test_result test_pattern_uniescape() {
    passgen_mem_accounting_t acc = passgen_mem_accounting_new(NULL);
    passgen_mem_t mem = passgen_mem_accounting(&acc);

    pattern_t pattern;
    pattern_result_t result;
    const char *str;
    pattern_segments_t *segments;
    pattern_segment_t *segment;
    pattern_range_t *range;

    str = "\\u{FC}[\\u{F6}\\u{fb}]";
    result = pattern_parse(&pattern, str, 0, &mem);
    assert(result.ok);
    segments = passgen_array_get(
        &pattern.group.segments,
        sizeof(pattern_segments_t),
        0);
    assert(segments);
    segment = passgen_array_get(&segments->items, sizeof(pattern_segment_t), 0);
    assert(segment);
    assert(segment->kind == PATTERN_CHAR);
    assert(segment->data.character.codepoint == 0xFC);
    segment = passgen_array_get(&segments->items, sizeof(pattern_segment_t), 1);
    assert(segment);
    assert(segment->kind == PATTERN_RANGE);
    assert(segment->data.range.items.len == 2);
    range = passgen_array_get(
        &segment->data.range.items,
        sizeof(pattern_range_t),
        0);
    assert(range->start == 0xF6);
    assert(range->end == 0xF6);
    range = passgen_array_get(
        &segment->data.range.items,
        sizeof(pattern_range_t),
        1);
    assert(range->start == 0xFb);
    assert(range->end == 0xFb);

    // free, make sure no memory leaks
    pattern_free(&pattern);
    assert(passgen_mem_accounting_check(&acc));
    passgen_mem_accounting_cleanup(&acc);

    return test_ok;
}

test_result test_pattern_pronouncable() {
    passgen_mem_accounting_t acc = passgen_mem_accounting_new(NULL);
    passgen_mem_t mem = passgen_mem_accounting(&acc);

    pattern_t pattern;
    pattern_result_t result;
    const char *str;
    pattern_segments_t *segments;
    pattern_segment_t *segment;
    pattern_special_t *special;

    str = "\\p";
    result = pattern_parse(&pattern, str, 0, &mem);
    assert(result.ok);
    segments = passgen_array_get(
        &pattern.group.segments,
        sizeof(pattern_segments_t),
        0);
    assert(segments);
    segment = passgen_array_get(&segments->items, sizeof(pattern_segment_t), 0);
    assert(segment);
    assert(segment->kind == PATTERN_SPECIAL);
    special = &segment->data.special;
    assert(special->pos.offset == 0);
    assert(special->pos.length == 2);
    assert(special->pos.length == 2);
    assert(special->kind == PATTERN_SPECIAL_PRONOUNCABLE);

    // free, make sure no memory leaks
    pattern_free(&pattern);
    assert(passgen_mem_accounting_check(&acc));
    passgen_mem_accounting_cleanup(&acc);

    return test_ok;
}

test_result test_pattern_somepatt() {
    passgen_mem_accounting_t acc = passgen_mem_accounting_new(NULL);
    passgen_mem_t mem = passgen_mem_accounting(&acc);

    pattern_t pattern;
    pattern_result_t result;
    const char *str;
    pattern_segments_t *segments;
    pattern_segment_t *segment;
    pattern_group_t *group;
    pattern_range_t *range;

    str = "(a|b)[dfg912]";
    result = pattern_parse(&pattern, str, 1, &mem);
    assert(result.ok);
    assert(pattern.mem == &mem);
    assert(pattern.pattern == str);
    assert(pattern.group.pos.offset == 0);
    assert(pattern.group.pos.length == 13);
    assert(pattern.group.segments.len == 1);
    segments = passgen_array_get(
        &pattern.group.segments,
        sizeof(pattern_segments_t),
        0);
    assert(segments);
    assert(segments->pos.offset == 0);
    assert(segments->pos.length == 13);
    assert(segments->items.len == 2);
    segment = passgen_array_get(&segments->items, sizeof(pattern_segment_t), 0);
    assert(segment);
    assert(segment->kind == PATTERN_GROUP);
    assert(segment->data.group.pos.offset == 1);
    assert(segment->data.group.pos.length == 3);
    assert(segment->data.group.segments.len == 2);
    group = &segment->data.group;
    segments =
        passgen_array_get(&group->segments, sizeof(pattern_segments_t), 0);
    assert(segments);
    assert(segments->pos.offset == 1);
    assert(segments->pos.length == 1);
    assert(segments->items.len == 1);
    segment = passgen_array_get(&segments->items, sizeof(pattern_segment_t), 0);
    assert(segment->kind == PATTERN_CHAR);
    assert(segment->data.character.pos.offset == 1);
    assert(segment->data.character.pos.length == 1);
    assert(segment->data.character.codepoint == 'a');
    segments =
        passgen_array_get(&group->segments, sizeof(pattern_segments_t), 1);
    assert(segments);
    assert(segments->pos.offset == 3);
    assert(segments->pos.length == 1);
    assert(segments->items.len == 1);
    segment = passgen_array_get(&segments->items, sizeof(pattern_segment_t), 0);
    assert(segment->kind == PATTERN_CHAR);
    assert(segment->data.character.pos.offset == 3);
    assert(segment->data.character.pos.length == 1);
    assert(segment->data.character.codepoint == 'b');
    segments = passgen_array_get(
        &pattern.group.segments,
        sizeof(pattern_segments_t),
        0);
    segment = passgen_array_get(&segments->items, sizeof(pattern_segment_t), 1);
    assert(segment);
    assert(segment->kind == PATTERN_RANGE);
    assert(segment->data.range.pos.offset == 5);
    assert(segment->data.range.pos.length == 8);
    assert(segment->data.range.items.len == 6);
    range = passgen_array_get(
        &segment->data.range.items,
        sizeof(pattern_range_t),
        0);
    assert(range->pos.offset == 6);
    assert(range->pos.length == 1);
    assert(range->start == 'd');
    assert(range->end == 'd');
    range = passgen_array_get(
        &segment->data.range.items,
        sizeof(pattern_range_t),
        1);
    assert(range->pos.offset == 7);
    assert(range->pos.length == 1);
    assert(range->start == 'f');
    assert(range->end == 'f');
    range = passgen_array_get(
        &segment->data.range.items,
        sizeof(pattern_range_t),
        2);
    assert(range->pos.offset == 8);
    assert(range->pos.length == 1);
    assert(range->start == 'g');
    assert(range->end == 'g');
    range = passgen_array_get(
        &segment->data.range.items,
        sizeof(pattern_range_t),
        3);
    assert(range->pos.offset == 9);
    assert(range->pos.length == 1);
    assert(range->start == '9');
    assert(range->end == '9');
    range = passgen_array_get(
        &segment->data.range.items,
        sizeof(pattern_range_t),
        4);
    assert(range->pos.offset == 10);
    assert(range->pos.length == 1);
    assert(range->start == '1');
    assert(range->end == '1');
    range = passgen_array_get(
        &segment->data.range.items,
        sizeof(pattern_range_t),
        5);
    assert(range->pos.offset == 11);
    assert(range->pos.length == 1);
    assert(range->start == '2');
    assert(range->end == '2');

    /* make sure choices list has the right entries */
    assert(segment->data.range.choices_list != NULL);
    assert(segment->data.range.choices_list[0] == 1);
    assert(segment->data.range.choices_list[1] == 2);
    assert(segment->data.range.choices_list[2] == 3);
    assert(segment->data.range.choices_list[3] == 4);
    assert(segment->data.range.choices_list[4] == 5);
    assert(segment->data.range.choices_list[5] == 6);

    // free, make sure no memory leaks
    pattern_free(&pattern);
    assert(passgen_mem_accounting_check(&acc));
    passgen_mem_accounting_cleanup(&acc);

    return test_ok;
}

/* TODO
 *
 * test "[abc\x{f6}]{100}" - weird behaviour
 */
