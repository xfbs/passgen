#include "passgen/pattern.h"
#include <stdbool.h>

const char *passgen_error_str[PATTERN_ERROR_LAST];

#define accept_regular(chr, error_kind, start)                    \
    token = pattern_token_peek(iter);                             \
    if(!passgen_token_regular(&token)) {                          \
        return pattern_token_error(start, iter->pos, error_kind); \
    }

bool pattern_group_is_separator(passgen_token_t token);
bool pattern_group_is_start(passgen_token_t token);
bool pattern_group_is_end(passgen_token_t token);

bool pattern_ranges_is_start(passgen_token_t token);
bool pattern_ranges_is_end(passgen_token_t token);
bool pattern_range_is_sep(passgen_token_t token);

bool pattern_segments_is_end(passgen_token_t token);

bool pattern_repeat_is_start(passgen_token_t token);
bool pattern_repeat_is_sep(passgen_token_t token);
bool pattern_repeat_is_end(passgen_token_t token);

bool pattern_sqarg_is_start(passgen_token_t token);
bool pattern_sqarg_is_end(passgen_token_t token);

pattern_result_t pattern_group_parse_inner(
    pattern_group_t *group,
    unicode_iter_t *iter,
    size_t depth,
    passgen_mem_t *mem);

pattern_result_t pattern_group_parse(
    pattern_group_t *group,
    unicode_iter_t *iter,
    size_t depth,
    passgen_mem_t *mem);

pattern_result_t pattern_ranges_parse_inner(
    pattern_ranges_t *range,
    unicode_iter_t *iter,
    passgen_mem_t *mem);

pattern_result_t pattern_ranges_parse(
    pattern_ranges_t *range,
    unicode_iter_t *iter,
    passgen_mem_t *mem);

pattern_result_t pattern_range_parse(
    pattern_range_t *item,
    unicode_iter_t *iter,
    passgen_mem_t *mem);

pattern_result_t pattern_segments_parse(
    pattern_segments_t *segment,
    unicode_iter_t *iter,
    size_t depth,
    passgen_mem_t *mem);

pattern_result_t pattern_segment_parse(
    pattern_segment_t *item,
    unicode_iter_t *iter,
    size_t depth,
    passgen_mem_t *mem);

pattern_result_t pattern_char_parse(
    pattern_char_t *character,
    unicode_iter_t *iter,
    passgen_mem_t *mem);

pattern_result_t pattern_special_parse(
    pattern_special_t *special,
    unicode_iter_t *iter,
    passgen_mem_t *mem);

static inline pattern_result_t
passgen_parse_number(size_t *number, unicode_iter_t *iter);

static pattern_result_t
pattern_parse_sqarg(pattern_substring_t *arg, unicode_iter_t *iter);
