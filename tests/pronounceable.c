#include "passgen/pronounceable.h"
#include "passgen/pronounceable_private.h"
#include "tests/tests.h"

test_result test_pronounceable_lists(void) {
    // TODO: test list properties (sorted, sums, etc).
    return test_ok;
}

test_result test_pronounceable_find2(void) {
    struct markov2 *list;

    list = passgen_pronounceable_find2(&passgen_pronounceable_english, '+');
    assert(list == NULL);

    list = passgen_pronounceable_find2(&passgen_pronounceable_english, 0);
    assert(list != NULL);
    assert(list->codepoint == 0);

    list = passgen_pronounceable_find2(&passgen_pronounceable_english, 'a');
    assert(list != NULL);
    assert(list->codepoint == 'a');

    return test_ok;
}

test_result test_pronounceable_find1(void) {
    struct markov1 *list;
    struct markov2 *list2;

    list2 = passgen_pronounceable_find2(&passgen_pronounceable_english, 0);
    assert(list2);

    list = passgen_pronounceable_find1(list2, '+');
    assert(list == NULL);

    list = passgen_pronounceable_find1(list2, 0);
    assert(list != NULL);
    assert(list->codepoint == 0);

    list = passgen_pronounceable_find1(list2, 'a');
    assert(list != NULL);
    assert(list->codepoint == 'a');

    return test_ok;
}
