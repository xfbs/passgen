#include "passgen/assert.h"
#include "passgen/config.h"
#include "passgen/random.h"
#include "passgen/util/endian.h"
#include <stdlib.h>
#include <string.h>

static size_t passgen_random_zero_read(void *context, void *dest, size_t size) {
    (void) context;
    memset(dest, 0, size);
    return size;
}

static void passgen_random_zero_close(void *context) {
    (void) context;
}

passgen_random *passgen_random_zero_open(passgen_random *random) {
    if(!random) {
        random = malloc(sizeof(passgen_random));
        if(!random) return NULL;
    }

    random->context = NULL;
    random->read = passgen_random_zero_read;
    random->close = passgen_random_zero_close;
    passgen_random_reload(random);
    return random;
}
