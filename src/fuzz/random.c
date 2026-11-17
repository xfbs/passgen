/// @file random.c
///
/// Fuzzing harness for descriptions of randomness sources, as passed to the
/// `--random` option. The input is the description.
#include <passgen/random.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static int has_prefix(const char *string, const char *prefix) {
    return 0 == strncmp(string, prefix, strlen(prefix));
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    char *description = malloc(size + 1);
    if(!description) {
        return 0;
    }
    memcpy(description, data, size);
    description[size] = 0;

    // these would open whatever file the input names, or use real
    // randomness, which makes runs irreproducible.
    if(!has_prefix(description, "file:") && !has_prefix(description, "system")) {
        passgen_random *random = passgen_random_open(NULL, description);
        if(random) {
            // more than one buffer
            for(size_t i = 0; i < 300; i++) {
                passgen_random_u64(random);
            }
            passgen_random_u8_max(random, 7);
            passgen_random_u64_max(random, 1);

            uint8_t large[2 * PASSGEN_RANDOM_BUFFER_LENGTH];
            passgen_random_read(random, large, sizeof(large));

            passgen_random_free(random);
        }
    }

    free(description);

    return 0;
}
