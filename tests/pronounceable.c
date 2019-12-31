#include "passgen/pronounceable.h"
#include "passgen/pronounceable_private.h"
#include "tests.h"

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

    for(size_t i = 0; i < 1000; i++) {
        int32_t buffer[64];
        size_t ret = passgen_pronounceable(
                PASSGEN_PRONOUNCEABLE_ENGLISH,
                rand,
                buffer,
                64);

        assert(ret > 0);
        assert(ret <= sizeof(buffer));
    }

    return test_ok;
}

/* rough test to see that there is a reasonable length distribution when
 * generating english words. */
test_result test_pronounceable_englendist(void) {
    random_t *rand = random_new();
    assert(rand);
    size_t lens[64] = {0};

    for(size_t i = 0; i < 1000; i++) {
        int32_t buffer[64];
        size_t ret = passgen_pronounceable(
                PASSGEN_PRONOUNCEABLE_ENGLISH,
                rand,
                buffer,
                64);

        assert(ret <= sizeof(buffer));
        lens[ret]++;
    }

    assert(lens[0] == 0);
    assert(lens[2] > 10);
    assert(lens[3] > 10);
    assert(lens[4] > 10);
    assert(lens[5] > 10);
    assert(lens[5] > 10);
    assert(lens[6] > 10);
    assert(lens[7] > 10);
    assert(lens[8] > 10);
    assert(lens[9] > 10);
    assert(lens[10] > 10);

    return test_ok;
}

/* rough test to see that there is a reasonable length distribution when
 * generating english words. */
test_result test_pronounceable_latlendist(void) {
    random_t *rand = random_new();
    assert(rand);
    size_t lens[64] = {0};

    for(size_t i = 0; i < 1000; i++) {
        int32_t buffer[64];
        size_t ret = passgen_pronounceable(
                PASSGEN_PRONOUNCEABLE_LATIN,
                rand,
                buffer,
                64);

        assert(ret > 0);
        assert(ret <= sizeof(buffer));

        lens[ret]++;
    }

    assert(lens[0] == 0);
    assert(lens[2] > 10);
    assert(lens[3] > 10);
    assert(lens[4] > 10);
    assert(lens[5] > 10);
    assert(lens[5] > 10);
    assert(lens[6] > 10);
    assert(lens[7] > 10);
    assert(lens[8] > 10);
    assert(lens[9] > 10);
    assert(lens[10] > 10);

    return test_ok;
}

test_result test_pronounceable_len(void) {
    random_t *rand = random_new();
    assert(rand);

    for(size_t i = 0; i < 1000; i++) {
        int32_t buffer[11];
        int ret = passgen_pronounceable_len(
                PASSGEN_PRONOUNCEABLE_ENGLISH,
                rand,
                buffer,
                8,
                11,
                0);

        assert(8 <= ret);
        assert(ret <= 11);
    }

    for(size_t i = 0; i < 1000; i++) {
        int32_t buffer[128];
        int ret = passgen_pronounceable_len(
                PASSGEN_PRONOUNCEABLE_ENGLISH,
                rand,
                buffer,
                100,
                120,
                10);

        assert(0 == ret);
    }

    return test_ok;
}

test_result test_pronounceable_engascii(void) {
    random_t *rand = random_new();
    assert(rand);

    for(size_t i = 0; i < 1000; i++) {
        int32_t buffer[64];
        size_t ret = passgen_pronounceable(
                PASSGEN_PRONOUNCEABLE_ENGLISH,
                rand,
                buffer,
                64);

        for(size_t i = 0; i < ret; i++) {
            assert('a' <= buffer[i]);
            assert(buffer[i] <= 'z');
        }
    }

    return test_ok;
}

test_result test_pronounceable_minascii(void) {
    random_t *rand = random_new();
    assert(rand);

    for(size_t i = 0; i < 1000; i++) {
        int32_t buffer[64];
        size_t ret = passgen_pronounceable_len(
                PASSGEN_PRONOUNCEABLE_ENGLISH,
                rand,
                buffer,
                7,
                10,
                0);

        for(size_t i = 0; i < ret; i++) {
            assert('a' <= buffer[i]);
            assert(buffer[i] <= 'z');
        }
    }

    return test_ok;
}

test_result test_pronounceable_lendist(void) {
    /* make sure that when specifying lengths [8, 11], you really can get all
     * possible lengths. */
    random_t *rand = random_new();
    assert(rand);

    size_t repeat = 1000;
    size_t lens[4] = {0, 0, 0, 0};

    for(size_t i = 0; i < repeat; i++) {
        int32_t buffer[11];
        size_t ret = passgen_pronounceable_len(
                PASSGEN_PRONOUNCEABLE_ENGLISH,
                rand,
                buffer,
                8,
                11,
                0);

        assert(8 <= ret);
        assert(ret <= 11);

        lens[ret - 8] += 1;
    }

    assert(80 < lens[0]);
    assert(80 < lens[1]);
    assert(80 < lens[2]);
    assert(80 < lens[3]);

    return test_ok;
}
