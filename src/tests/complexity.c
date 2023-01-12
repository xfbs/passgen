#include "passgen/parser/parser.h"
#include "passgen/parser/token.h"
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
#include "tests.h"
#include <math.h>
#include <passgen/generate.h>
#include <passgen/passgen.h>

double complexity(const char *p) {
    // parse pattern
    struct passgen_pattern pattern;
    passgen_error error;
    if(0 != passgen_parse(&pattern, &error, p)) {
        return NAN;
    }

    size_t buffer_len = 256;
    uint8_t buffer[buffer_len];
    buffer[0] = 0;
    passgen_random random;
    passgen_random_open(&random);
    struct passgen_env env;
    env.random = &random;
    env.find_complexity = true;
    env.complexity = 0.0;
    int length =
        passgen_generate_fill_utf8(&pattern, &env, &buffer[0], buffer_len);
    buffer[length] = 0;
    passgen_pattern_free(&pattern);

    return env.complexity;
}

bool equals(double a, double b) {
    double epsilon = 0.00001;
    return (a - epsilon) < b && (a + epsilon) > b;
}

test_result test_complexity_single_choice(void) {
    assert(equals(0.0, 0.0));
    assert(equals(1.0, complexity("")));
    assert(equals(1.0, complexity("a")));
    assert(equals(1.0, complexity("abc")));

    return test_ok;
}

test_result test_complexity_dual_choice(void) {
    assert(equals(2.0, complexity("[ab]")));
    assert(equals(2.0, complexity("(this|that)")));
    assert(equals(2.0, complexity("a{1,2}")));

    return test_ok;
}

test_result test_complexity_triple_choice(void) {
    assert(equals(3.0, complexity("[abc]")));
    assert(equals(3.0, complexity("[abc]{1}")));
    assert(equals(3.0, complexity("(this|that|other)")));
    assert(equals(3.0, complexity("(this|that|other){1}")));
    assert(equals(3.0, complexity("a{1,3}")));
    assert(equals(3.0, complexity("(a{1,3})")));
    assert(equals(3.0, complexity("(a{1,3}){1}")));

    return test_ok;
}

test_result test_complexity_compound(void) {
    assert(equals(27.0, complexity("[abc]{3}")));
    assert(equals(27.0, complexity("(this|that|other){3}")));
    assert(equals(27.0, complexity("(a{1,3}){3}")));

    return test_ok;
}
