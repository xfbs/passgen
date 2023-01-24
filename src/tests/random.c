#include "passgen/util/random.h"

#include <stdbool.h>
#include <stdio.h>

#include "tests.h"

#define SEED 234720984723

test_result test_random_u8(void) {
    passgen_random *rand = passgen_random_new_xorshift(SEED);
    assert(rand);

    // generate random nubers until we got almost all of them.
    bool gotten[UINT8_MAX + 1] = {false};
    for(size_t i = 0; i < (32 * UINT8_MAX); ++i) {
        gotten[passgen_random_u8(rand)] = true;
    }

    // there's still a (255/256)^(8*256) = 0.03% chance this fails.
    for(size_t i = 0; i < UINT8_MAX; ++i) {
        assert(gotten[i]);
    }

    passgen_random_free(rand);

    return test_ok;
}

test_result test_random_u8_max(void) {
    passgen_random *rand = passgen_random_new_xorshift(SEED);
    assert(rand);

    for(size_t max = 1; max < UINT8_MAX; ++max) {
        // generate random nubers until we got almost all of them.
        bool gotten[UINT8_MAX] = {false};
        for(size_t i = 0; i < (16 * UINT8_MAX); ++i) {
            uint8_t r = passgen_random_u8_max(rand, max);
            assert(r < max);
            gotten[r] = true;
        }

        // there's still a (255/256)^(8*256) = 0.03% chance this fails.
        for(size_t i = 0; i < max; ++i) {
            assert(gotten[i]);
        }
    }

    passgen_random_free(rand);

    return test_ok;
}

test_result test_random_u16(void) {
    passgen_random *rand = passgen_random_new_xorshift(SEED);
    assert(rand);

    // generate random nubers until we got almost all of them.
    bool gotten[UINT16_MAX + 1] = {false};
    for(size_t i = 0; i < (32 * UINT16_MAX); ++i) {
        gotten[passgen_random_u16(rand)] = true;
    }

    // there's still a (255/256)^(8*256) = 0.03% chance this fails.
    for(size_t i = 0; i < UINT16_MAX; ++i) {
        assert(gotten[i]);
    }

    passgen_random_free(rand);

    return test_ok;
}

test_result test_random_u16_max(void) {
    passgen_random *rand = passgen_random_new_xorshift(SEED);
    assert(rand);

    size_t max[] = {
        1,
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
            assert(r < max[n]);
            gotten[r] = true;
        }

        for(size_t i = 0; i < max[n]; ++i) {
            assert(gotten[i]);
        }
    }

    passgen_random_free(rand);

    return test_ok;
}

test_result test_random_u32_max(void) {
    passgen_random *rand = passgen_random_new_xorshift(SEED);
    assert(rand);

    for(size_t i = 1; i < 1000000; i++) {
        uint32_t max = passgen_random_u32(rand);

        assert(passgen_random_u32_max(rand, max) < max);
    }

    passgen_random_free(rand);

    return test_ok;
}

test_result test_random_u64_max(void) {
    passgen_random *rand = passgen_random_new_xorshift(SEED);
    assert(rand);

    for(size_t i = 1; i < 1000000; i++) {
        uint32_t max = passgen_random_u64(rand);

        assert(passgen_random_u64_max(rand, max) < max);
    }

    passgen_random_free(rand);

    return test_ok;
}

test_result test_random_new(void) {
    passgen_random *random = passgen_random_new(NULL);
    assert(random);
    assert(random->read);
    assert(random->close);
    passgen_random_free(random);

    return test_ok;
}

test_result test_random_new_path(void) {
    passgen_random *random;
    random = passgen_random_new_path("/dev/nonexistent");
    assert(!random);

    // reading from /dev/zero should always yield zero.
    random = passgen_random_new_path("/dev/zero");
    assert(random);
    assert(random->data);
    assert(passgen_random_u8(random) == 0);
    assert(passgen_random_u16(random) == 0);
    assert(passgen_random_u32(random) == 0);
    assert(passgen_random_u64(random) == 0);
    passgen_random_free(random);

    return test_ok;
}

test_result test_random_open(void) {
    passgen_random random;
    assert(passgen_random_open(&random, NULL));
    assert(random.read);
    assert(random.close);
    passgen_random_close(&random);
    assert(!random.read);

    return test_ok;
}

test_result test_random_open_path(void) {
    passgen_random random;
    assert(!passgen_random_open_path(&random, "/dev/nonexistent"));

    assert(passgen_random_open_path(&random, "/dev/zero"));
    assert(random.data);
    assert(passgen_random_u8(&random) == 0);
    assert(passgen_random_u16(&random) == 0);
    assert(passgen_random_u32(&random) == 0);
    assert(passgen_random_u64(&random) == 0);
    passgen_random_close(&random);
    assert(!random.data);

    return test_ok;
}

test_result test_random_read(void) {
    passgen_random random;
    assert(passgen_random_open_xorshift(&random, SEED));

    uint8_t data[2000] = {0};

    // fill small.
    passgen_random_read(&random, &data[0], 1);
    assert(random.pos == 1);
    assert(data[0] == random.buffer[0]);
    assert(data[1] == 0);

    passgen_random_read(&random, &data[0], 2);
    assert(random.pos == 3);
    assert(data[0] == random.buffer[1]);
    assert(data[1] == random.buffer[2]);
    assert(data[2] == 0);

    passgen_random_read(&random, &data[0], 4);
    assert(random.pos == 7);
    assert(data[0] == random.buffer[3]);
    assert(data[1] == random.buffer[4]);
    assert(data[2] == random.buffer[5]);
    assert(data[3] == random.buffer[6]);
    assert(data[4] == 0);

    passgen_random_read(&random, &data[0], 8);
    assert(random.pos == 15);
    assert(data[0] == random.buffer[7]);
    assert(data[1] == random.buffer[8]);
    assert(data[2] == random.buffer[9]);
    assert(data[3] == random.buffer[10]);
    assert(data[4] == random.buffer[11]);
    assert(data[5] == random.buffer[12]);
    assert(data[6] == random.buffer[13]);
    assert(data[7] == random.buffer[14]);
    assert(data[8] == 0);

    // test wraparound.
    while(random.pos != (sizeof(random.buffer) - 1)) {
        passgen_random_read(&random, &data[0], 1);
    }
    passgen_random_read(&random, &data[0], 1);
    assert(random.pos == 0);

    while(random.pos != (sizeof(random.buffer) - 1)) {
        passgen_random_read(&random, &data[0], 1);
    }
    passgen_random_read(&random, &data[0], 3);
    assert(data[1] == random.buffer[0]);
    assert(data[2] == random.buffer[1]);
    assert(random.pos == 2);

    // test reading larger.
    passgen_random_read(&random, &data[0], sizeof(random.buffer) + 1);
    assert(random.pos == 2);

    passgen_random_close(&random);

    return test_ok;
}

test_result test_random_xorshift(void) {
    passgen_random random;

    // using a state of zero generates only zeroes.
    assert(passgen_random_open_xorshift(&random, 0) != NULL);
    assert(passgen_random_u8(&random) == 0);
    assert(passgen_random_u16(&random) == 0);
    assert(passgen_random_u32(&random) == 0);
    assert(passgen_random_u64(&random) == 0);
    passgen_random_close(&random);

    // same seed always yields the same output
    assert(passgen_random_open_xorshift(&random, 123) != NULL);
    assert(passgen_random_u8(&random) == 187);
    assert(passgen_random_u16(&random) == 31102);
    assert(passgen_random_u32(&random) == 7933);
    assert(passgen_random_u64(&random) == 2214108778545186304ULL);
    passgen_random_close(&random);

    return test_ok;
}

#undef SEED
