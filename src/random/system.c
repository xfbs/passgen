#include "passgen/assert.h"
#include "passgen/config.h"
#include "passgen/random.h"
#include "passgen/util/endian.h"
#include <stdlib.h>
#include <string.h>

#ifdef __linux__
#define PASSGEN_RANDOM_HAVE_SYSTEM
#include <errno.h>
#include <sys/random.h>
#include <sys/types.h>

static size_t
passgen_random_system_read(void *context, void *dest, size_t size) {
    (void) context;
    // getrandom() may return fewer bytes than requested (for requests larger
    // than 256 bytes) or fail with EINTR, so keep going until we have it all.
    size_t done = 0;
    while(done < size) {
        ssize_t ret = getrandom((uint8_t *) dest + done, size - done, 0);
        if(ret < 0) {
            if(errno == EINTR) {
                continue;
            }
            break;
        }
        done += (size_t) ret;
    }
    return done;
}
#endif

#ifdef __APPLE__
#define PASSGEN_RANDOM_HAVE_SYSTEM

static size_t
passgen_random_system_read(void *context, void *dest, size_t size) {
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
    return random;
#else
    static const char *device = "/dev/urandom";
    random = passgen_random_path_open(random, device);
    if(!random) {
        fprintf(
            stderr,
            "error: cannot open system randomness device '%s'\n",
            device);
    }
    return random;
#endif
}

passgen_random *
passgen_random_path_open(passgen_random *random, const char *path) {
    FILE *device = fopen(path, "r");
    if(!device) {
        return NULL;
    }

    if(!random) {
        random = malloc(sizeof(passgen_random));
        if(!random) {
            fclose(device);
            return NULL;
        }
    }

    return passgen_random_file_open(random, device);
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
