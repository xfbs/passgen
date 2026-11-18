#include "passgen/assert.h"
#include "passgen/config.h"
#include "passgen/random.h"
#include "passgen/util/alloc.h"
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

// Fills the buffer for the first time. If that does not work, the source is
// closed (and released, if it was allocated here).
static passgen_random *
passgen_random_start(passgen_random *random, bool allocated) {
    random->failed = false;
    passgen_random_reload(random);

    if(random->failed) {
        random->close(random->context);
        if(allocated) {
            passgen_free(random, sizeof(passgen_random));
        }
        return NULL;
    }

    return random;
}

passgen_random *passgen_random_system_open(passgen_random *random) {
#ifdef PASSGEN_RANDOM_HAVE_SYSTEM
    bool allocated = !random;
    if(!random) {
        random = passgen_alloc(sizeof(passgen_random));
        if(!random) return NULL;
    }

    random->context = NULL;
    random->read = passgen_random_system_read;
    random->close = passgen_random_system_close;
    return passgen_random_start(random, allocated);
#else
    // whoever opens it says why, if it cannot be opened.
    return passgen_random_path_open(random, "/dev/urandom");
#endif
}

passgen_random *
passgen_random_path_open(passgen_random *random, const char *path) {
    FILE *device = fopen(path, "r");
    if(!device) {
        return NULL;
    }

    return passgen_random_file_open(random, device);
}

passgen_random *passgen_random_file_open(passgen_random *random, FILE *file) {
    bool allocated = !random;
    if(!random) {
        random = passgen_alloc(sizeof(passgen_random));
        if(!random) {
            fclose(file);
            return NULL;
        }
    }

    random->context = file;
    random->read = passgen_random_read_file;
    random->close = passgen_random_close_file;
    return passgen_random_start(random, allocated);
}
