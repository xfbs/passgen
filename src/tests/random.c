#include "passgen/random.h"
#include "passgen/config.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "tests.h"

/// Tests that a given function covers all possible outputs (0..max, inclusive).
#define TEST_COVERAGE(max, collate, function)                    \
    do {                                                         \
        size_t coverage_len = ((size_t) max) / (collate) + 1ULL; \
        uint8_t *coverage = calloc((coverage_len + 7) / 8, 1);   \
        bool full_coverage = false;                              \
        while(!full_coverage) {                                  \
            for(size_t i = 0; i < 256; i++) {                    \
                size_t pos = function / (collate);               \
                coverage[pos / 8] |= 1 << (pos % 8);             \
            }                                                    \
            full_coverage = true;                                \
            for(size_t i = 0; i <= (max / (collate)); i++) {     \
                if(!(coverage[i / 8] & (1 << (i % 8)))) {        \
                    full_coverage = false;                       \
                    break;                                       \
                }                                                \
            }                                                    \
        }                                                        \
        free(coverage);                                          \
    } while(false)

double standard_deviation(size_t count, uint32_t *elements) {
    (void) count;
    (void) elements;
    // TODO: determine deviation
    return 0;
}

/// Tests that a given function has an even distribution
#define TEST_DISTRIBUTION(max, bucket_num, target, function)   \
    uint32_t *buckets = calloc(bucket_num, sizeof(uint32_t));  \
    while(true) {                                              \
        size_t bucket = function / ((max) / (bucket_num) + 1); \
        buckets[bucket] += 1;                                  \
        if(buckets[bucket] == target) {                        \
            break;                                             \
        }                                                      \
    }                                                          \
    assert(standard_deviation(bucket_num, buckets) < 10);      \
    free(buckets)

test_result test_random_u8(void) {
    passgen_random random;
    assert(passgen_random_open(&random, NULL));

    TEST_COVERAGE(UINT8_MAX, 1, passgen_random_u8(&random));

    passgen_random_close(&random);

    return test_ok;
}

test_result test_random_u8_max(void) {
    passgen_random random;
    assert(passgen_random_open(&random, NULL));

    for(size_t max = 1; max < UINT8_MAX; max++) {
        TEST_COVERAGE(max - 1, 1, passgen_random_u8_max(&random, max));
    }

    passgen_random_close(&random);

    return test_ok;
}

test_result test_random_u16(void) {
    passgen_random random;
    assert(passgen_random_open(&random, NULL));

    TEST_COVERAGE(UINT16_MAX, 1, passgen_random_u16(&random));

    passgen_random_close(&random);

    return test_ok;
}

test_result test_random_u16_max(void) {
    passgen_random random;
    assert(passgen_random_open(&random, NULL));

    for(size_t max = 1; max < UINT8_MAX; max++) {
        TEST_COVERAGE(max - 1, 1, passgen_random_u16_max(&random, max));
    }

    for(size_t max = 1; max < UINT16_MAX; max *= 3) {
        TEST_COVERAGE(max - 1, 1, passgen_random_u16_max(&random, max));
    }

    passgen_random_close(&random);

    return test_ok;
}

test_result test_random_u32(void) {
    passgen_random random;
    assert(passgen_random_open(&random, NULL));

    TEST_COVERAGE(UINT32_MAX, 1 << 16, passgen_random_u32(&random));
    TEST_DISTRIBUTION(
        UINT32_MAX,
        1 << 10,
        1 << 10,
        passgen_random_u32(&random));

    passgen_random_close(&random);

    return test_ok;
}

test_result test_random_u32_max(void) {
    passgen_random random;
    assert(passgen_random_open(&random, NULL));

    for(size_t max = 1; max < UINT8_MAX; max++) {
        TEST_COVERAGE(max - 1, 1, passgen_random_u32_max(&random, max));
    }

    for(size_t max = 1; max < UINT16_MAX; max *= 3) {
        TEST_COVERAGE(max - 1, 1, passgen_random_u32_max(&random, max));
    }

    passgen_random_close(&random);

    return test_ok;
}

test_result test_random_u64(void) {
    passgen_random random;
    assert(passgen_random_open(&random, NULL));

    // FIXME
    // TEST_COVERAGE(UINT64_MAX, 1ULL << 48, 1024, passgen_random_u32(&random));
    TEST_DISTRIBUTION(
        UINT64_MAX,
        1 << 10,
        1 << 10,
        passgen_random_u64(&random));

    passgen_random_close(&random);

    return test_ok;
}

test_result test_random_u64_max(void) {
    passgen_random random;
    assert(passgen_random_open(&random, NULL));

    for(size_t max = 1; max < UINT8_MAX; max++) {
        TEST_COVERAGE(max - 1, 1, passgen_random_u64_max(&random, max));
    }

    for(size_t max = 1; max < UINT16_MAX; max *= 3) {
        TEST_COVERAGE(max - 1, 1, passgen_random_u64_max(&random, max));
    }

    for(size_t i = 1; i < 1000000; i++) {
        uint32_t max = passgen_random_u64(&random);

        assert(passgen_random_u64_max(&random, max) < max);
    }

    passgen_random_close(&random);

    return test_ok;
}

test_result test_random_new(void) {
    passgen_random *random_default = passgen_random_open(NULL, NULL);
    assert(random_default);
    assert(random_default->read);
    assert(random_default->close);

    passgen_random *random = passgen_random_open(NULL, "system");
    assert(random);
    assert_eq(random->read, random_default->read);
    assert_eq(random->close, random_default->close);
    passgen_random_free(random);
    passgen_random_free(random_default);

    random = passgen_random_open(NULL, "zero");
    assert(random);
    assert(random->read);
    assert(random->close);
    assert_eq(passgen_random_u8(random), 0);
    assert_eq(passgen_random_u16(random), 0);
    assert_eq(passgen_random_u32(random), 0);
    assert_eq(passgen_random_u64(random), 0);
    passgen_random_free(random);

    random = passgen_random_open(NULL, "xorshift:1234");
    assert(random);
    assert(random->read);
    assert(random->close);
    assert_eq(passgen_random_u8(random), 91);
    assert_eq(passgen_random_u16(random), 11632);
    assert_eq(passgen_random_u32(random), 79584);
    assert_eq(passgen_random_u64(random), 3801598356675656448ULL);
    assert_eq(passgen_random_u64_max(random, 999999999), 851051297);
    passgen_random_free(random);

    random = passgen_random_open(NULL, "file:/dev/zero");
    assert(random);
    assert(random->read);
    assert(random->close);
    assert_eq(passgen_random_u8(random), 0);
    assert_eq(passgen_random_u16(random), 0);
    assert_eq(passgen_random_u32(random), 0);
    assert_eq(passgen_random_u64(random), 0);
    passgen_random_free(random);

    assert(!passgen_random_open(NULL, "other"));
    assert(!passgen_random_open(NULL, "xorshift:abc"));
    assert(!passgen_random_open(NULL, "xorshift:"));
    assert(!passgen_random_open(NULL, "file:"));
    assert(!passgen_random_open(NULL, "file:/dev/nonexistant"));

    return test_ok;
}

test_result test_random_file(void) {
    FILE *file = fopen("/dev/zero", "r");
    assert(file);
    passgen_random *random = passgen_random_file_open(NULL, file);
    assert(random);
    assert(random->read);
    assert(random->close);
    assert_eq(passgen_random_u8(random), 0);
    assert_eq(passgen_random_u16(random), 0);
    assert_eq(passgen_random_u32(random), 0);
    assert_eq(passgen_random_u64(random), 0);
    passgen_random_free(random);

    return test_ok;
}

test_result test_random_zero(void) {
    passgen_random *random = passgen_random_zero_open(NULL);
    assert(random);
    assert(random->read);
    assert(random->close);
    assert_eq(passgen_random_u8(random), 0);
    assert_eq(passgen_random_u16(random), 0);
    assert_eq(passgen_random_u32(random), 0);
    assert_eq(passgen_random_u64(random), 0);
    passgen_random_free(random);

    return test_ok;
}

test_result test_random_new_path(void) {
    passgen_random *random;
    random = passgen_random_path_open(NULL, "/dev/nonexistent");
    assert(!random);

    // reading from /dev/zero should always yield zero.
    random = passgen_random_path_open(NULL, "/dev/zero");
    assert(random);
    assert(random->context);
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

    assert(passgen_random_open(&random, "system"));
    assert(random.read);
    assert(random.close);
    passgen_random_close(&random);

    assert(passgen_random_open(&random, "xorshift:1234"));
    assert(random.read);
    assert(random.close);
    passgen_random_close(&random);

    assert(passgen_random_open(&random, "zero"));
    assert(random.read);
    assert(random.close);
    passgen_random_close(&random);

    assert(passgen_random_open(&random, "file:/dev/random"));
    assert(random.read);
    assert(random.close);
    passgen_random_close(&random);

    assert(!passgen_random_open(&random, "other"));
    assert(!passgen_random_open(&random, "file:/dev/notexists"));
    assert(!passgen_random_open(&random, "xorshift:"));
    assert(!passgen_random_open(&random, "xorshift:abc"));

    return test_ok;
}

test_result test_random_path_open(void) {
    passgen_random random;
    assert(!passgen_random_path_open(&random, "/dev/nonexistent"));

    assert(passgen_random_path_open(&random, "/dev/zero"));
    assert(random.context);
    assert(passgen_random_u8(&random) == 0);
    assert(passgen_random_u16(&random) == 0);
    assert(passgen_random_u32(&random) == 0);
    assert(passgen_random_u64(&random) == 0);
    passgen_random_close(&random);
    assert(!random.context);

    return test_ok;
}

#define XORSHIFT_SEED 234720984723

test_result test_random_read(void) {
    passgen_random random;
    assert(passgen_random_xorshift_open(&random, XORSHIFT_SEED));

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

#undef XORSHIFT_SEED

test_result test_random_xorshift(void) {
    passgen_random random;

    // using a state of zero generates only zeroes.
    assert(passgen_random_xorshift_open(&random, 0) != NULL);
    assert(passgen_random_u8(&random) == 0);
    assert(passgen_random_u16(&random) == 0);
    assert(passgen_random_u32(&random) == 0);
    assert(passgen_random_u64(&random) == 0);
    passgen_random_close(&random);

    // same seed always yields the same output
    assert(passgen_random_xorshift_open(&random, 123) != NULL);
    assert(passgen_random_u8(&random) == 187);
    assert(passgen_random_u16(&random) == 31102);
    assert(passgen_random_u32(&random) == 7933);
    assert(passgen_random_u64(&random) == 2214108778545186304ULL);
    passgen_random_close(&random);

    return test_ok;
}

#define CHECK_BEGINNING(random)                  \
    assert(passgen_random_u8(random) == 180);    \
    assert(passgen_random_u16(random) == 43073); \
    assert(passgen_random_u32(random) == 1604870485)

#define CHECK_REST(random)                                         \
    assert(passgen_random_u64(random) == 5567227278517054982ULL);  \
    assert(passgen_random_u64(random) == 10120912194545613486ULL); \
    assert(passgen_random_u64(random) == 14168488844181147231ULL); \
    assert(passgen_random_u64(random) == 17990605897407475310ULL); \
    assert(passgen_random_u64(random) == 14488897527995456554ULL); \
    assert(passgen_random_u64(random) == 16603445463307625220ULL); \
    assert(passgen_random_u64(random) == 17826730983279974707ULL); \
    assert(passgen_random_u64(random) == 7745812445609307379ULL);  \
    assert(passgen_random_u64(random) == 6756294678665739130ULL)

#ifdef PASSGEN_MONOCYPHER
test_result test_random_chacha20_seek(void) {
    passgen_random random;

    assert(
        passgen_random_chacha20_open(
            &random,
            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
            "passgen") != NULL);
    CHECK_BEGINNING(&random);
    CHECK_REST(&random);

    // seek back to beginning, same values
    passgen_random_chacha20_seek(&random, 0);
    CHECK_BEGINNING(&random);
    CHECK_REST(&random);

    // seek to middle of stream, only latter values
    passgen_random_chacha20_seek(&random, 7);
    CHECK_REST(&random);

    passgen_random_close(&random);

    return test_ok;
}

#undef CHECK_BEGINNING
#undef CHECK_REST

test_result test_random_chacha20(void) {
    passgen_random random;

    assert(
        passgen_random_chacha20_open(
            &random,
            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
            "passgen") != NULL);
    assert(passgen_random_u8(&random) == 180);
    assert(passgen_random_u16(&random) == 43073);
    assert(passgen_random_u32(&random) == 1604870485);
    assert(passgen_random_u64(&random) == 5567227278517054982ULL);
    passgen_random_close(&random);

    assert(
        passgen_random_chacha20_open(
            &random,
            "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb",
            "passgen") != NULL);
    assert(passgen_random_u8(&random) == 14);
    assert(passgen_random_u16(&random) == 19281);
    assert(passgen_random_u32(&random) == 2608689089);
    assert(passgen_random_u64(&random) == 12744604773769097796ULL);
    passgen_random_close(&random);

    return test_ok;
}

test_result test_random_chacha20_argon2(void) {
    passgen_random random;

    assert(
        passgen_random_chacha20_argon2_open(
            &random,
            "mypassword",
            "google.com",
            "",
            NULL) != NULL);
    assert(passgen_random_u8(&random) == 53);
    assert(passgen_random_u16(&random) == 45909);
    assert(passgen_random_u32(&random) == 1750958599);
    assert(passgen_random_u64(&random) == 13270789079271667756ULL);
    passgen_random_close(&random);

    return test_ok;
}

#endif
