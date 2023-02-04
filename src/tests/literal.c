#include "passgen/pattern/literal.h"
#include "tests.h"

test_result test_literal_init(void) {
    passgen_pattern_literal literal = {0};
    memset(&literal, 1, sizeof(literal));

    passgen_pattern_literal_init(&literal);
    assert_eq(literal.count, 0);
    assert_eq(literal.tainted, 0);

    return test_ok;
}

test_result test_literal_append(void) {
    passgen_pattern_literal literal;
    const int32_t codepoints[] =
        {0xabcd, 0x1234, 0x65ac, 0xdaa0, 0xdead, 0xbeef, 0xcafe, 0};

    passgen_pattern_literal_init(&literal);
    assert_eq(literal.count, 0);
    assert_eq(literal.tainted, 0);

    for(size_t i = 0; i < 7; i++) {
        assert_eq(passgen_pattern_literal_append(&literal, codepoints[i]), 0);
        assert_eq(literal.count, i + 1);
        assert_eq(literal.tainted, 0);

        for(size_t j = 0; j <= i; j++) {
            assert_eq(literal.codepoints[j], codepoints[j]);
        }
    }

    return test_ok;
}

test_result test_literal_append_full(void) {
    passgen_pattern_literal literal;
    const int32_t codepoint = 0xabcd;

    passgen_pattern_literal_init(&literal);
    assert_eq(literal.count, 0);
    assert_eq(literal.tainted, 0);

    for(size_t i = 0; i < 7; i++) {
        assert_eq(passgen_pattern_literal_append(&literal, codepoint), 0);
    }

    assert_eq(passgen_pattern_literal_append(&literal, codepoint), 1);
    assert_eq(literal.count, 7);
    assert_eq(literal.tainted, 0);

    return test_ok;
}

test_result test_literal_taint(void) {
    passgen_pattern_literal literal;

    passgen_pattern_literal_init(&literal);
    assert_eq(literal.count, 0);
    assert_eq(literal.tainted, 0);

    passgen_pattern_literal_taint(&literal);
    assert_eq(literal.count, 0);
    assert_eq(literal.tainted, 1);

    return test_ok;
}

test_result test_literal_append_taint(void) {
    passgen_pattern_literal literal;
    const int32_t codepoint = 0xabcd;

    passgen_pattern_literal_init(&literal);
    assert_eq(literal.count, 0);
    assert_eq(literal.tainted, 0);

    for(size_t i = 0; i < 5; i++) {
        assert_eq(passgen_pattern_literal_append(&literal, codepoint), 0);
    }

    passgen_pattern_literal_taint(&literal);
    assert_eq(literal.count, 5);
    assert_eq(literal.tainted, 1);

    assert_eq(passgen_pattern_literal_append(&literal, codepoint), 1);
    assert_eq(literal.count, 5);
    assert_eq(literal.tainted, 1);

    return test_ok;
}
