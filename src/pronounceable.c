#include "passgen/debug.h"
#include "passgen/assert.h"
#include "passgen/pronounceable.h"
#include "passgen/pronounceable_private.h"
#include <stdlib.h>

struct markov *markov_pronounceable_all[] = {
    &passgen_pronounceable_english,
    NULL
};

size_t passgen_pronounceable2(
        struct markov2 *list,
        random_t *rand,
        char *buf,
        size_t len)
{
    return 0;
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
    struct markov *list = markov_pronounceable_all[type];
    assert(list);

    return passgen_pronounceable2(list->list, rand, buf, len);
}

int
passgen_pronounceable_len(
        enum passgen_pronounceable_type type,
        random_t *rand,
        char *buf,
        size_t len,
        size_t tries)
{
    return 0;
}

int32_t passgen_pronounceable_choose2(struct markov2 *list, int32_t a, int32_t b) {
    return 0;
}

static int
markov1_find(const void *key_p, const void *item_p)
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

struct markov1 *
passgen_pronounceable_find1(
        struct markov2 *list,
        int32_t codepoint)
{
    struct markov1 *result;

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

struct markov2 *
passgen_pronounceable_find2(
        struct markov *list,
        int32_t codepoint)
{
    struct markov2 *result;

    result = bsearch(
            &codepoint,
            list->list,
            list->list_len,
            sizeof(struct markov2),
            markov_find);

    return result;
}

struct markov0 *
passgen_pronounceable_find(
        struct markov1 *list,
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
