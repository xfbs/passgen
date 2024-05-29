#include "passgen/assert.h"
#include "passgen/config.h"
#include "passgen/random.h"
#include "passgen/util/endian.h"
#include <stdlib.h>
#include <string.h>

size_t passgen_random_read_zero(void *dest, size_t size, void *data) {
    (void) data;
    memset(dest, 0, size);
    return size;
}

void passgen_random_close_zero(void *data) {
    (void) data;
}

passgen_random *passgen_random_open_zero(passgen_random *random) {
    if(!random) {
        random = malloc(sizeof(passgen_random));
        if(!random) return NULL;
    }

    random->data = NULL;
    random->read = passgen_random_read_zero;
    random->close = passgen_random_close_zero;
    passgen_random_reload(random);
    return random;
}

passgen_random *passgen_random_new_zero() {
    passgen_random *random = malloc(sizeof(passgen_random));
    if(!random) return NULL;
    return passgen_random_open_zero(random);
}
