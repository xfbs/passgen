#include "passgen/debug.h"
#include "passgen/assert.h"
#include "passgen/pronounceable.h"
#include "passgen/pronounceable_private.h"
#include <stdlib.h>

struct markov *markov_pronounceable_all[] = {
    &passgen_pronounceable_english,
    NULL
};

size_t
passgen_pronounceable(
        enum passgen_pronounceable_type type,
        random_t *rand,
        char *buf,
        size_t len)
{
    return 0;
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

struct markov1 *
passgen_pronounceable_find1(
        struct markov2 *list,
        int32_t codepoint)
{
    return NULL;
}

struct markov2 *
passgen_pronounceable_find2(
        struct markov *list,
        int32_t codepoint)
{
    return NULL;
}
