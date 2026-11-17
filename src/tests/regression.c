/// @file regression.c
///
/// Regression tests for bugs that were found and fixed. Most of these are
/// only really meaningful when run with sanitizers enabled.
#include "tests.h"
#include <passgen/random.h>
#include <string.h>

/// Reads that straddle the end of the internal buffer must return the
/// remaining buffered bytes followed by fresh ones, rather than handing out
/// bytes from the start of the buffer a second time.
test_result test_regression_random_read_boundary(void) {
    passgen_random chunked, bytewise;
    assert(passgen_random_xorshift_open(&chunked, 1234));
    assert(passgen_random_xorshift_open(&bytewise, 1234));

    size_t offset = PASSGEN_RANDOM_BUFFER_LENGTH - 4;
    uint8_t expected[PASSGEN_RANDOM_BUFFER_LENGTH + 4];
    for(size_t i = 0; i < sizeof(expected); i++) {
        expected[i] = passgen_random_u8(&bytewise);
    }

    uint8_t skip[PASSGEN_RANDOM_BUFFER_LENGTH];
    uint8_t straddle[8];
    passgen_random_read(&chunked, skip, offset);
    passgen_random_read(&chunked, straddle, sizeof(straddle));

    assert(0 == memcmp(skip, expected, offset));
    assert(0 == memcmp(straddle, &expected[offset], sizeof(straddle)));

    // exactly emptying the buffer must work too.
    passgen_random_close(&chunked);
    assert(passgen_random_xorshift_open(&chunked, 1234));
    passgen_random_read(&chunked, skip, offset);
    passgen_random_read(&chunked, straddle, 4);
    assert(0 == memcmp(straddle, &expected[offset], 4));
    assert(passgen_random_u8(&chunked) == expected[offset + 4]);

    passgen_random_close(&chunked);
    passgen_random_close(&bytewise);

    return test_ok;
}

#ifdef PASSGEN_MONOCYPHER
/// Overlong keys and passphrases in randomness descriptions used to overflow
/// fixed-size stack buffers.
test_result test_regression_random_open_overlong(void) {
    char desc[1024];
    passgen_random random;

    // 32 bytes of key is fine, 33 is not.
    strcpy(desc, "chacha20:");
    memset(desc + strlen(desc), 'A', 32);
    desc[9 + 32] = 0;
    assert(passgen_random_open(&random, desc));
    passgen_random_close(&random);

    desc[9 + 32] = 'A';
    desc[9 + 33] = 0;
    assert(!passgen_random_open(&random, desc));

    memset(desc, 'A', sizeof(desc) - 1);
    desc[sizeof(desc) - 1] = 0;
    memcpy(desc, "chacha20:", 9);
    assert(!passgen_random_open(&random, desc));

    // same for the fields of chacha20-argon2.
    memset(desc, 'A', sizeof(desc) - 1);
    memcpy(desc, "chacha20-argon2:", 16);
    assert(!passgen_random_open(&random, desc));

    strcpy(desc, "chacha20-argon2:pass:");
    memset(desc + strlen(desc), 'B', 500);
    desc[21 + 500] = 0;
    assert(!passgen_random_open(&random, desc));

    assert(passgen_random_open(&random, "chacha20-argon2:pass:domain:token"));
    passgen_random_close(&random);

    return test_ok;
}
#endif
