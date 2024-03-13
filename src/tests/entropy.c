#include "tests.h"
#include <math.h>
#include <passgen/generate.h>
#include <passgen/passgen.h>

double entropy(const char *p) {
    // parse pattern
    passgen_pattern pattern;
    passgen_error error;
    if(0 != passgen_parse(&pattern, &error, p)) {
        return NAN;
    }

    size_t buffer_len = 256;
    uint8_t buffer[buffer_len];
    buffer[0] = 0;
    passgen_random random;
    passgen_random_open(&random, NULL);
    passgen_env env;
    passgen_env_init(&env, &random);
    double entropy;
    int length = passgen_generate_fill_utf8(
        &pattern,
        &env,
        &entropy,
        &buffer[0],
        buffer_len);
    buffer[length] = 0;
    passgen_pattern_free(&pattern);

    return entropy;
}

bool equals(double a, double b) {
    double epsilon = 0.00001;
    return (a - epsilon) < b && (a + epsilon) > b;
}

test_result test_entropy_literal(void) {
    assert(equals(0.0, 0.0));
    assert(equals(1.0, entropy("")));
    assert(equals(1.0, entropy("a")));
    assert(equals(1.0, entropy("ab")));
    assert(equals(1.0, entropy("abc")));
    assert(equals(1.0, entropy("abcdefghijklmnopqrstuvwxyz")));

    return test_ok;
}

test_result test_entropy_group(void) {
    assert(equals(1.0, entropy("()")));
    assert(equals(1.0, entropy("(abc)")));
    assert(equals(1.0, entropy("(())")));
    assert(equals(2.0, entropy("(abc|def)")));
    assert(equals(3.0, entropy("(abc|def|hij)")));
    assert(equals(4.0, entropy("(abc|def|hij|klm)")));

    return test_ok;
}

test_result test_entropy_range(void) {
    assert(equals(1.0, entropy("[a]")));
    assert(equals(2.0, entropy("[ab]")));
    assert(equals(3.0, entropy("[abc]")));
    assert(equals(3.0, entropy("[a-c]")));
    assert(equals(26.0, entropy("[a-z]")));
    assert(equals(10.0, entropy("[0-9]")));
    assert(equals(16.0, entropy("[0-9a-f]")));
    assert(equals(20.0, entropy("[0-9a-f,.:\\-]")));

    return test_ok;
}

test_result test_entropy_repeat(void) {
    assert(equals(3.0, entropy("[abc]{1}")));
    assert(equals(3.0 * 3.0, entropy("[abc]{2}")));
    assert(equals(3.0 * 3.0 * 3.0, entropy("[abc]{3}")));

    assert(equals(3.0, entropy("(this|that|other){1}")));
    assert(equals(3.0 * 3.0, entropy("(this|that|other){2}")));
    assert(equals(3.0 * 3.0 * 3.0, entropy("(this|that|other){3}")));

    assert(equals(1.0, entropy("a{1}")));
    assert(equals(2.0, entropy("a{1,2}")));
    assert(equals(3.0, entropy("a{1,3}")));

    assert(equals(1.0, entropy("(a){1}")));
    assert(equals(2.0, entropy("(a){1,2}")));
    assert(equals(3.0, entropy("(a){1,3}")));

    return test_ok;
}
