#include "tests.h"

test_result test_array_init(void);
test_result test_array_push(void);
test_result test_array_get(void);
test_result test_array_pop(void);
test_result test_memory_stdlib(void);
test_result test_memory_limited(void);
test_result test_memory_limited_malloc(void);
test_result test_memory_accounting(void);
test_result test_parser_empty(void);
test_result test_parser_single_char(void);
test_result test_parser_multi_char(void);
test_result test_parser_multi_groups(void);
test_result test_pattern_parse_str();
test_result test_pattern_parse_group();
test_result test_pattern_parse_strrep();
test_result test_pattern_parse_rangerep();
test_result test_pattern_parse_orstr();
test_result test_pattern_parse_range();
test_result test_pattern_escapable();
test_result test_pattern_utf8();
test_result test_pattern_uniescape();
test_result test_pattern_pronouncable();
test_result test_pattern_somepatt();
test_result test_pattern_random_simple();
test_result test_pronounceable_lists(void);
test_result test_pronounceable_find2(void);
test_result test_pronounceable_find1(void);
test_result test_pronounceable_find(void);
test_result test_pronounceable(void);
test_result test_pronounceable_englendist(void);
test_result test_pronounceable_latlendist(void);
test_result test_pronounceable_len(void);
test_result test_pronounceable_engascii(void);
test_result test_pronounceable_minascii(void);
test_result test_pronounceable_lendist(void);
test_result test_random_uint8(void);
test_result test_random_uint8_max(void);
test_result test_random_uint16(void);
test_result test_random_uint16_max(void);
test_result test_random_uint32_max(void);
test_result test_random_uint64_max(void);
test_result test_random_new(void);
test_result test_random_new_path(void);
test_result test_random_open(void);
test_result test_random_open_path(void);
test_result test_random_read(void);
test_result test_reader_string();
test_result test_reader_file();
test_result test_passgen_token_next(void);
test_result test_passgen_token_peek(void);
test_result test_passgen_token_types(void);
test_result test_passgen_token_errors(void);
test_result test_token_normal(void);
test_result test_token_escaped(void);
test_result test_token_unicode(void);
test_result test_token_unicode_error_start(void);
test_result test_token_unicode_error_payload(void);
test_result test_token_unicode_error_len(void);
test_result test_unicode_iter_sized(void);
test_result test_unicode_iter_peek(void);
test_result test_unicode_iter_error(void);
test_result test_unicode_iter_next(void);
test_result test_utf8_decode_simple(void);
test_result test_utf8_decode_short_output(void);
test_result test_utf8_encode_simple(void);

test_entry tests[] = {
    test(array_init),
    test(array_push),
    test(array_get),
    test(array_pop),
    test(memory_stdlib),
    test(memory_limited),
    test(memory_limited_malloc),
    test(memory_accounting),
    test(parser_empty),
    test(parser_single_char),
    test(parser_multi_char),
    test(parser_multi_groups),
    test(pattern_parse_str),
    test(pattern_parse_group),
    test(pattern_parse_strrep),
    test(pattern_parse_rangerep),
    test(pattern_parse_orstr),
    test(pattern_parse_range),
    test(pattern_escapable),
    test(pattern_utf8),
    test(pattern_uniescape),
    test(pattern_pronouncable),
    test(pattern_somepatt),
    test(pattern_random_simple),
    test(pronounceable_lists),
    test(pronounceable_find2),
    test(pronounceable_find1),
    test(pronounceable_find),
    test(pronounceable),
    test(pronounceable_englendist),
    test(pronounceable_latlendist),
    test(pronounceable_len),
    test(pronounceable_engascii),
    test(pronounceable_minascii),
    test(pronounceable_lendist),
    test(random_uint8),
    test(random_uint8_max),
    test(random_uint16),
    test(random_uint16_max),
    test(random_uint32_max),
    test(random_uint64_max),
    test(random_new),
    test(random_new_path),
    test(random_open),
    test(random_open_path),
    test(random_read),
    test(reader_string),
    test(reader_file),
    test(passgen_token_next),
    test(passgen_token_peek),
    test(passgen_token_types),
    test(passgen_token_errors),
    test(token_normal),
    test(token_escaped),
    test(token_unicode),
    test(token_unicode_error_start),
    test(token_unicode_error_payload),
    test(token_unicode_error_len),
    test(unicode_iter_sized),
    test(unicode_iter_peek),
    test(unicode_iter_error),
    test(unicode_iter_next),
    test(utf8_decode_simple),
    test(utf8_decode_short_output),
    test(utf8_encode_simple),
    {NULL, NULL},
};
