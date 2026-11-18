/// @file random.c
///
/// Fuzzing harness for descriptions of randomness sources, as passed to the
/// `--random` option. The input is the description.
///
/// The description is parsed and opened separately, so that keys are derived
/// from master passphrases with small Argon2 parameters. With the usual ones,
/// this takes a second, and nearly all of the time would be spent in
/// Monocypher, rather than in what this is about.
#include "common.h"
#include <passgen/random.h>
#include <passgen/random_desc.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static void run(const uint8_t *data, size_t size) {
    char *description = malloc(size + 1);
    if(!description) {
        return;
    }
    memcpy(description, data, size);
    description[size] = 0;

    // files would open whatever file the input names, and the system source
    // uses real randomness, which makes runs irreproducible.
    passgen_random_desc desc;
    if(passgen_random_desc_parse(&desc, description) &&
       desc.kind != PASSGEN_RANDOM_DESC_FILE &&
       desc.kind != PASSGEN_RANDOM_DESC_SYSTEM) {
#ifdef PASSGEN_MONOCYPHER
        if(desc.kind == PASSGEN_RANDOM_DESC_CHACHA20_ARGON2) {
            static const passgen_argon2_params params = {
                .memory = 8,
                .passes = 1,
                .lanes = 1,
            };
            desc.data.chacha20_argon2.params = &params;
        }
#endif
        passgen_random *random = passgen_random_desc_open(NULL, &desc);
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
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    fuzz_run(run, data, size);

    return 0;
}
