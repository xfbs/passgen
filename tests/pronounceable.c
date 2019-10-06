#include "passgen/pronounceable.h"
#include "passgen/pronounceable_private.h"
#include "tests/tests.h"

test_result test_pronounceable_lists(void) {
    // TODO: test list properties (sorted, sums, etc).

    return test_ok;
}

test_result test_pronounceable_find2(void) {
    const struct markov2 *list;

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
    const struct markov1 *list;
    const struct markov2 *list2;

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

test_result test_pronounceable_find(void) {
    const struct markov0 *choice;
    const struct markov1 *list;
    const struct markov2 *list2;

    list2 = passgen_pronounceable_find2(&passgen_pronounceable_english, 0);
    assert(list2);

    list = passgen_pronounceable_find1(list2, 0);
    assert(list != NULL);
    assert(list->codepoint == 0);

    choice = passgen_pronounceable_find(list, list->frequency_sum);
    assert(choice == NULL);

    choice = passgen_pronounceable_find(list, 0);
    assert(choice != NULL);
    assert(choice->codepoint == 'a');

    choice = passgen_pronounceable_find(list, list->frequency_sum - 1);
    assert(choice != NULL);
    assert(choice->codepoint == 'z');

    return test_ok;
}

test_result test_pronounceable(void) {
    random_t *rand = random_new();
    assert(rand);

    char buffer[10];
    size_t ret = passgen_pronounceable(
            PASSGEN_PRONOUNCEABLE_ENGLISH,
            rand,
            buffer,
            sizeof buffer);

    assert(ret > 0);

    return test_ok;
}
