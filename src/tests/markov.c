#include "passgen/markov.h"
#include "tests.h"
#include <stdlib.h>

test_result test_markov_add(void) {
    passgen_markov markov;
    passgen_markov_init(&markov, 1);

    passgen_markov_add(&markov, &(const uint32_t[]){'a'}, 1, 1);
    passgen_markov_add(&markov, &(const uint32_t[]){'b'}, 1, 1);
    passgen_markov_add(&markov, &(const uint32_t[]){'c'}, 1, 1);

    passgen_markov_free(&markov);

    passgen_markov_init(&markov, 2);

    passgen_markov_add(&markov, &(const uint32_t[]){'a', 'b'}, 2, 1);
    passgen_markov_add(&markov, &(const uint32_t[]){'b'}, 1, 1);
    passgen_markov_add(&markov, &(const uint32_t[]){'c', 'd', 'e', 'f'}, 4, 1);

    passgen_markov_free(&markov);

    return test_ok;
}
