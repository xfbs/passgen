#include <criterion/criterion.h>
//#include <criterion/new/assert.h>
#include "passgen/random.h"

#include <stdbool.h>
#include <stdio.h>

#define SEED 234720984723

Test(random, u8) {
    passgen_random_t *rand = passgen_random_new_xorshift(SEED);
    cr_assert(rand != NULL);

    // generate random numbers until we got almost all of them.
    bool gotten[UINT8_MAX + 1] = {false};
    for(size_t i = 0; i < (32 * UINT8_MAX); ++i) {
        gotten[passgen_random_u8(rand)] = true;
    }

    // there's still a (255/256)^(8*256) = 0.03% chance this fails.
    for(size_t i = 0; i < UINT8_MAX; ++i) {
        cr_assert(gotten[i]);
    }

    passgen_random_free(rand);
}

Test(random, u8_max) {
    passgen_random_t *rand = passgen_random_new_xorshift(SEED);
    cr_assert(rand != NULL);

    for(size_t max = 1; max < UINT8_MAX; ++max) {
        // generate random nubers until we got almost all of them.
        bool gotten[UINT8_MAX] = {false};
        for(size_t i = 0; i < (16 * UINT8_MAX); ++i) {
            uint8_t r = passgen_random_u8_max(rand, max);
            cr_assert(r < max);
            gotten[r] = true;
        }

        // there's still a (255/256)^(8*256) = 0.03% chance this fails.
        for(size_t i = 0; i < max; ++i) {
            cr_assert(gotten[i]);
        }
    }

    passgen_random_free(rand);
}

Test(random, u16) {
    passgen_random_t *rand = passgen_random_new_xorshift(SEED);
    cr_assert(rand != NULL);

    // generate random nubers until we got almost all of them.
    bool gotten[UINT16_MAX + 1] = {false};
    for(size_t i = 0; i < (32 * UINT16_MAX); ++i) {
        gotten[passgen_random_u16(rand)] = true;
    }

    // there's still a (255/256)^(8*256) = 0.03% chance this fails.
    for(size_t i = 0; i < UINT16_MAX; ++i) {
        cr_assert(gotten[i]);
    }

    passgen_random_free(rand);
}

Test(random, u16_max) {
    passgen_random_t *rand = passgen_random_new_xorshift(SEED);
    cr_assert(rand != NULL);

    size_t max[] = {1,
                    2,
                    3,
                    4,
                    5,
                    100,
                    200,
                    500,
                    1000,
                    1500,
                    2000,
                    5000,
                    10000,
                    15000,
                    20000,
                    30000,
                    45000,
                    60000,
                    0};

    for(size_t n = 1; max[n]; ++n) {
        bool gotten[UINT16_MAX] = {false};
        for(size_t i = 0; i < (16 * max[n]); ++i) {
            uint16_t r = passgen_random_u16_max(rand, max[n]);
            cr_assert(r < max[n]);
            gotten[r] = true;
        }

        for(size_t i = 0; i < max[n]; ++i) {
            cr_assert(gotten[i]);
        }
    }

    passgen_random_free(rand);
}

Test(random, u32) {
    passgen_random_t *rand = passgen_random_new_xorshift(SEED);
    cr_assert(rand != NULL);

    for(size_t i = 1; i < 1000000; i++) {
        uint32_t max = passgen_random_u32(rand);

        cr_assert(passgen_random_u32_max(rand, max) < max);
    }

    passgen_random_free(rand);
}

Test(random, u64_max) {
    passgen_random_t *rand = passgen_random_new_xorshift(SEED);
    cr_assert(rand != NULL);

    for(size_t i = 1; i < 1000000; i++) {
        uint32_t max = passgen_random_u64(rand);

        cr_assert(passgen_random_u64_max(rand, max) < max);
    }

    passgen_random_free(rand);
}

Test(random, new) {
    passgen_random_t *random = passgen_random_new();
    cr_assert(random != NULL);
    cr_assert(random->read != NULL);
    cr_assert(random->close != NULL);
    passgen_random_free(random);
}

#ifdef __linux__
Test(random, new_path_nonexistent) {
    passgen_random_t *random = passgen_random_new_path("/dev/nonexistent");
    cr_assert(random == NULL);
}
#endif

#ifdef __linux__
Test(random, new_path_zero) {
    // reading from /dev/zero should always yield zero.
    passgen_random_t *random = passgen_random_new_path("/dev/zero");
    cr_assert(random != NULL);
    cr_assert(random->data != NULL);
    cr_assert(passgen_random_u8(random) == 0);
    cr_assert(passgen_random_u16(random) == 0);
    cr_assert(passgen_random_u32(random) == 0);
    cr_assert(passgen_random_u64(random) == 0);
    passgen_random_free(random);
}
#endif

Test(random, open) {
    passgen_random_t random;
    cr_assert(passgen_random_open(&random) != NULL);
    cr_assert(random.read != NULL);
    cr_assert(random.close != NULL);
    passgen_random_close(&random);
    cr_assert(random.read == NULL);
}

#ifdef __linux__
Test(random, open_path_nonexistent) {
    passgen_random_t random;
    cr_assert(passgen_random_open_path(&random, "/dev/nonexistent") == NULL);
}
#endif

#ifdef __linux__
Test(random, open_path_zero) {
    passgen_random_t random;
    cr_assert(passgen_random_open_path(&random, "/dev/zero") != NULL);
    cr_assert(random.data != NULL);
    cr_assert(passgen_random_u8(&random) == 0);
    cr_assert(passgen_random_u16(&random) == 0);
    cr_assert(passgen_random_u32(&random) == 0);
    cr_assert(passgen_random_u64(&random) == 0);
    passgen_random_close(&random);
    cr_assert(random.data == NULL);
}
#endif

Test(random, read) {
    passgen_random_t random;
    cr_assert(passgen_random_open_xorshift(&random, SEED) != NULL);

    uint8_t data[2000] = {0};

    // fill small.
    passgen_random_read(&random, &data[0], 1);
    cr_assert(random.pos == 1);
    cr_assert(data[0] == random.buffer[0]);
    cr_assert(data[1] == 0);

    passgen_random_read(&random, &data[0], 2);
    cr_assert(random.pos == 3);
    cr_assert(data[0] == random.buffer[1]);
    cr_assert(data[1] == random.buffer[2]);
    cr_assert(data[2] == 0);

    passgen_random_read(&random, &data[0], 4);
    cr_assert(random.pos == 7);
    cr_assert(data[0] == random.buffer[3]);
    cr_assert(data[1] == random.buffer[4]);
    cr_assert(data[2] == random.buffer[5]);
    cr_assert(data[3] == random.buffer[6]);
    cr_assert(data[4] == 0);

    passgen_random_read(&random, &data[0], 8);
    cr_assert(random.pos == 15);
    cr_assert(data[0] == random.buffer[7]);
    cr_assert(data[1] == random.buffer[8]);
    cr_assert(data[2] == random.buffer[9]);
    cr_assert(data[3] == random.buffer[10]);
    cr_assert(data[4] == random.buffer[11]);
    cr_assert(data[5] == random.buffer[12]);
    cr_assert(data[6] == random.buffer[13]);
    cr_assert(data[7] == random.buffer[14]);
    cr_assert(data[8] == 0);

    // test wraparound.
    while(random.pos != (sizeof(random.buffer) - 1)) {
        passgen_random_read(&random, &data[0], 1);
    }
    passgen_random_read(&random, &data[0], 1);
    cr_assert(random.pos == 0);

    while(random.pos != (sizeof(random.buffer) - 1)) {
        passgen_random_read(&random, &data[0], 1);
    }
    passgen_random_read(&random, &data[0], 3);
    cr_assert(data[1] == random.buffer[0]);
    cr_assert(data[2] == random.buffer[1]);
    cr_assert(random.pos == 2);

    // test reading larger.
    passgen_random_read(&random, &data[0], sizeof(random.buffer) + 1);
    cr_assert(random.pos == 2);

    passgen_random_close(&random);
}

Test(random, xorshift) {
    passgen_random_t random;

    // using a state of zero generates only zeroes.
    cr_assert(passgen_random_open_xorshift(&random, 0) != NULL);
    cr_assert(passgen_random_u8(&random) == 0);
    cr_assert(passgen_random_u16(&random) == 0);
    cr_assert(passgen_random_u32(&random) == 0);
    cr_assert(passgen_random_u64(&random) == 0);
    passgen_random_close(&random);

    // same seed always yields the same output
    cr_assert(passgen_random_open_xorshift(&random, 123) != NULL);
    cr_assert(passgen_random_u8(&random) == 187);
    cr_assert(passgen_random_u16(&random) == 31102);
    cr_assert(passgen_random_u32(&random) == 7933);
    cr_assert(passgen_random_u64(&random) == 2214108778545186304ULL);
    passgen_random_close(&random);
}

#undef SEED
