#include "passgen/container/array.h"
#include "passgen/container/hashmap.h"
#include "passgen/container/stack.h"
#include "passgen/markov.h"
#include "passgen/parser/parser.h"
#include "passgen/pattern/group.h"
#include "passgen/pattern/literal.h"
#include "passgen/pattern/pattern.h"
#include "passgen/pattern/range.h"
#include "passgen/pattern/repeat.h"
#include "passgen/pattern/segment.h"
#include "passgen/pattern/segment_item.h"
#include "passgen/pattern/set.h"
#include "passgen/pattern/special.h"
#include "passgen/wordlist.h"
#include "tests.h"
#include <string.h>

test_result test_sizes_builtin(void) {
    assert_eq(sizeof(void *), 8);
    assert_eq(sizeof(size_t), 8);
    return test_ok;
}

test_result test_sizes_parser(void) {
    assert_eq(sizeof(passgen_parser), 48);
    assert_eq(sizeof(passgen_parser_state), 24);
    return test_ok;
}

test_result test_sizes_tokenizer(void) {
    assert_eq(sizeof(passgen_token), 24);
    assert_eq(sizeof(passgen_token_parser), 40);
    return test_ok;
}

test_result test_sizes_pattern(void) {
    assert_eq(sizeof(passgen_pattern), 40);
    assert_eq(sizeof(passgen_pattern_repeat), 16);
    assert_eq(sizeof(passgen_pattern_group), 40);
    assert_eq(sizeof(passgen_pattern_segment), 40);
    assert_eq(sizeof(passgen_pattern_item), 72);
    assert_eq(sizeof(passgen_pattern_range), 16);
    assert_eq(sizeof(passgen_pattern_literal), 32);
    assert_eq(sizeof(passgen_pattern_set), 40);
    assert_eq(sizeof(passgen_pattern_special), 24);
    return test_ok;
}

test_result test_sizes_wordlist(void) {
    assert_eq(sizeof(passgen_markov), 24);
    assert_eq(sizeof(passgen_markov_leaf), 16);
    assert_eq(sizeof(passgen_markov_node), 8);
    assert_eq(sizeof(passgen_wordlist), 72);
    return test_ok;
}

test_result test_sizes_util(void) {
    assert_eq(sizeof(passgen_hashmap), 32);
    assert_eq(sizeof(passgen_hashmap_context), 16);
    assert_eq(sizeof(passgen_hashmap_entry), 16);
    assert_eq(sizeof(passgen_stack), 32);
    assert_eq(sizeof(passgen_random), 1024 + 4 * 8);
    assert_eq(sizeof(passgen_array), 24);
    return test_ok;
}
