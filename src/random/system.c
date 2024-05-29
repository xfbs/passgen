#include "passgen/assert.h"
#include "passgen/config.h"
#include "passgen/random.h"
#include "passgen/util/endian.h"
#include <stdlib.h>
#include <string.h>

static const char *passgen_random_default_device = "/dev/urandom";

#ifdef __linux__
#define PASSGEN_RANDOM_HAVE_SYSTEM
#include <sys/random.h>

static size_t passgen_random_system_read(void *context, void *dest, size_t size) {
    (void) context;
    return getrandom(dest, size, 0);
}
#endif

#ifdef __APPLE__
#define PASSGEN_RANDOM_HAVE_SYSTEM

static size_t passgen_random_system_read(void *context, void *dest, size_t size) {
    (void) context;
    arc4random_buf(dest, size);
    return size;
}
#endif

static size_t passgen_random_read_file(void *context, void *dest, size_t size) {
    return fread(dest, 1, size, context);
}

static void passgen_random_close_file(void *context) {
    fclose(context);
}

static void passgen_random_system_close(void *context) {
    (void) context;
}

passgen_random *passgen_random_system_open(passgen_random *random) {
#ifdef PASSGEN_RANDOM_HAVE_SYSTEM
    if(!random) {
        random = malloc(sizeof(passgen_random));
        if(!random) return NULL;
    }

    random->context = NULL;
    random->read = passgen_random_system_read;
    random->close = passgen_random_system_close;
    passgen_random_reload(random);
#else
    passgen_random *rand =
        passgen_random_path_open(random, passgen_random_default_device);
    if(!rand) {
        fprintf(
            stderr,
            "error: cannot open system randomness device '%s'\n",
            passgen_random_default_device);
    }
#endif
    return random;
}

passgen_random *
passgen_random_path_open(passgen_random *random, const char *path) {
    FILE *device = fopen(path, "r");
    if(!device) {
        return NULL;
    }

    if(!random) {
        random = malloc(sizeof(passgen_random));
        if(!random) return NULL;
    }

    return passgen_random_file_open(random, device);

    passgen_random_reload(random);

    return random;
}

passgen_random *passgen_random_file_open(passgen_random *random, FILE *file) {
    if(!random) {
        random = malloc(sizeof(passgen_random));
        if(!random) return NULL;
    }

    random->context = file;
    random->read = passgen_random_read_file;
    random->close = passgen_random_close_file;
    passgen_random_reload(random);
    return random;
}
