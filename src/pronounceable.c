#include "passgen/debug.h"
#include "passgen/assert.h"
#include "passgen/pronounceable.h"
#include "passgen/pronounceable_private.h"
#include <stdlib.h>

const struct markov *markov_pronounceable_all[] = {
    &passgen_pronounceable_english,
    NULL
};

const struct markov0 *
passgen_pronounceable_choose(
        const struct markov *list,
        random_t *rand,
        int32_t p1,
        int32_t p2)
{
    const struct markov2 *list2 = passgen_pronounceable_find2(list, p1);

    if(!list2) {
        return NULL;
    }

    const struct markov1 *list1 = passgen_pronounceable_find1(list2, p2);

    if(!list1) {
        return NULL;
    }

    size_t choices = list1->frequency_sum;
    size_t nchoice = random_uint64_max(rand, choices);

    return passgen_pronounceable_find(list1, nchoice);
}

size_t passgen_pronounceable2(
        const struct markov *list,
        random_t *rand,
        char *buf,
        size_t len)
{
    int32_t p1 = 0, p2 = 0;
    size_t pos = 0;

    do {
        const struct markov2 *list2 = passgen_pronounceable_find2(list, p1);
        assert(list2);

        const struct markov1 *list1 = passgen_pronounceable_find1(list2, p2);
        assert(list1);

        size_t choices = list1->frequency_sum;
        size_t nchoice = random_uint64_max(rand, choices);

        const struct markov0 *choice = passgen_pronounceable_find(list1, nchoice);
        assert(choice);

        /* TODO: emit utf8 from the codepoint? */
        buf[pos] = choice->codepoint;
        pos += 1;

        p1 = p2;
        p2 = choice->codepoint;
    } while(p2 && pos < len);

    /* null-terminate string if there is space. */
    if(pos < len) {
        buf[pos] = 0;
    }

    return pos;
}

size_t
passgen_pronounceable(
        enum passgen_pronounceable_type type,
        random_t *rand,
        char *buf,
        size_t len)
{
    assert(type < PASSGEN_PRONOUNCEABLE_LAST);

    /* get list */
    const struct markov *list = markov_pronounceable_all[type];
    assert(list);

    return passgen_pronounceable2(list, rand, buf, len);
}

int
passgen_pronounceable_len(
        enum passgen_pronounceable_type type,
        random_t *rand,
        char *buf,
        size_t len,
        size_t min,
        size_t max,
        size_t tries)
{
    assert(type < PASSGEN_PRONOUNCEABLE_LAST);

    /* get list */
    const struct markov *list = markov_pronounceable_all[type];
    assert(list);

    /* try tries times (or infinite times if tries is 0) to fill buffer up with
     * a word. return 0 on success. */
    for(size_t i = 0; !tries || i < tries; i++) {
        size_t ret = passgen_pronounceable2(list, rand, buf, len);

        if(ret == (len - 1)) {
            return 0;
        }
    }

    return -1;
}

static int
markov1_find(
        const void *key_p,
        const void *item_p)
{
    const int32_t *key = key_p;
    const struct markov1 *item = item_p;

    if(*key < item->codepoint) {
        return -1;
    } else if(*key > item->codepoint) {
        return 1;
    } else {
        return 0;
    }
}

const struct markov1 *
passgen_pronounceable_find1(
        const struct markov2 *list,
        int32_t codepoint)
{
    const struct markov1 *result;

    result = bsearch(
            &codepoint,
            list->list,
            list->list_len,
            sizeof(struct markov1),
            markov1_find);

    return result;
}

static int
markov_find(const void *key_p, const void *item_p)
{
    const int32_t *key = key_p;
    const struct markov2 *item = item_p;

    if(*key < item->codepoint) {
        return -1;
    } else if(*key > item->codepoint) {
        return 1;
    } else {
        return 0;
    }
}

const struct markov2 *
passgen_pronounceable_find2(
        const struct markov *list,
        int32_t codepoint)
{
    const struct markov2 *result;

    result = bsearch(
            &codepoint,
            list->list,
            list->list_len,
            sizeof(struct markov2),
            markov_find);

    return result;
}

const struct markov0 *
passgen_pronounceable_find(
        const struct markov1 *list,
        size_t choice)
{
    for(size_t i = 0; i < list->list_len; i++) {
        if(choice < list->list[i].frequency) {
            return &list->list[i];
        }

        choice -= list->list[i].frequency;
    }

    return NULL;
}
