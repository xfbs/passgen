#include "passgen/markov.h"
#include "tests.h"
#include <stdlib.h>

test_result test_markov_add(void) {
    passgen_markov *markov = malloc(sizeof(passgen_markov));
    passgen_markov_init(markov, 3);

    markov = passgen_markov_add(markov, &(const uint32_t[]){'b', 'o', 'o', 't', 's'}, 5, 1);

    passgen_markov_free(markov);

    return test_ok;
}
