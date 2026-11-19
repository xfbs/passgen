#include "passgen/random.h"
#include "passgen/config.h"
#include "passgen/random_desc.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stats.h"
#include "testalloc.h"
#include "tests.h"

#ifndef _WIN32
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

/// A ChaCha20 key in hex: the bytes 0 to 31.
#define KEY "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f"

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

/// How many numbers are drawn to check how they are distributed.
#define DISTRIBUTION_SAMPLES 200000

/// Seed for checking how numbers are distributed, so that every run draws the
/// same ones.
#define DISTRIBUTION_SEED 0x5eed5eed

/// Draws a number below max (or of the full width, with zero).
typedef uint64_t draw_func(passgen_random *random, uint64_t max);

static uint64_t draw_u8(passgen_random *random, uint64_t max) {
    return max ? passgen_random_u8_max(random, (uint8_t) max)
               : passgen_random_u8(random);
}

static uint64_t draw_u16(passgen_random *random, uint64_t max) {
    return max ? passgen_random_u16_max(random, (uint16_t) max)
               : passgen_random_u16(random);
}

static uint64_t draw_u32(passgen_random *random, uint64_t max) {
    return max ? passgen_random_u32_max(random, (uint32_t) max)
               : passgen_random_u32(random);
}

static uint64_t draw_u64(passgen_random *random, uint64_t max) {
    return max ? passgen_random_u64_max(random, max)
               : passgen_random_u64(random);
}

/// Whether the numbers which a function draws below max are spread evenly.
/// The range is split into at most 64 buckets of the same size (apart from
/// the last one, if it does not divide evenly), and each has to come up as
/// often as its size says.
static bool distribution_fits(draw_func *draw, uint64_t max) {
    size_t buckets = max < 64 ? (size_t) max : 64;
    uint64_t size = max / buckets + (max % buckets != 0);
    size_t used = (size_t) ((max - 1) / size + 1);

    uint64_t counts[64] = {0};
    double weights[64];
    for(size_t i = 0; i < used; i++) {
        uint64_t start = i * size;
        weights[i] = (double) (max - start < size ? max - start : size);
    }

    passgen_random random;
    if(!passgen_random_xorshift_open(&random, DISTRIBUTION_SEED)) {
        return false;
    }

    bool okay = true;
    for(size_t i = 0; okay && i < DISTRIBUTION_SAMPLES; i++) {
        uint64_t value = draw(&random, max);
        okay = value < max;
        if(okay) {
            counts[value / size] += 1;
        }
    }
    passgen_random_close(&random);

    return okay && chi_squared_fits(used, counts, weights);
}

/// Whether numbers of the full width are spread evenly, by their top eight
/// bits.
static bool full_distribution_fits(draw_func *draw, unsigned bits) {
    uint64_t counts[256] = {0};
    double weights[256];
    for(size_t i = 0; i < 256; i++) {
        weights[i] = 1;
    }

    passgen_random random;
    if(!passgen_random_xorshift_open(&random, DISTRIBUTION_SEED)) {
        return false;
    }
    for(size_t i = 0; i < DISTRIBUTION_SAMPLES; i++) {
        counts[draw(&random, 0) >> (bits - 8)] += 1;
    }
    passgen_random_close(&random);

    return chi_squared_fits(256, counts, weights);
}

/// Numbers are spread evenly, also for maximums which do not divide the
/// range evenly, where taking the remainder would make some numbers more
/// likely than others.
test_result test_random_distribution(void) {
    assert(full_distribution_fits(draw_u8, 8));
    assert(full_distribution_fits(draw_u16, 16));
    assert(full_distribution_fits(draw_u32, 32));
    assert(full_distribution_fits(draw_u64, 64));

    const uint64_t u8_maxes[] = {2, 3, 6, 100, 192, UINT8_MAX};
    for(size_t i = 0; i < sizeof(u8_maxes) / sizeof(u8_maxes[0]); i++) {
        assert(distribution_fits(draw_u8, u8_maxes[i]));
    }

    const uint64_t u16_maxes[] = {3, 1000, 40000, UINT16_MAX};
    for(size_t i = 0; i < sizeof(u16_maxes) / sizeof(u16_maxes[0]); i++) {
        assert(distribution_fits(draw_u16, u16_maxes[i]));
    }

    const uint64_t u32_maxes[] = {3, 1000000, 3000000000u, UINT32_MAX};
    for(size_t i = 0; i < sizeof(u32_maxes) / sizeof(u32_maxes[0]); i++) {
        assert(distribution_fits(draw_u32, u32_maxes[i]));
    }

    const uint64_t u64_maxes[] =
        {3, 1000000000000ull, 0xc000000000000000ull, UINT64_MAX};
    for(size_t i = 0; i < sizeof(u64_maxes) / sizeof(u64_maxes[0]); i++) {
        assert(distribution_fits(draw_u64, u64_maxes[i]));
    }

    return test_ok;
}

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

    // every one of 2^16 equal parts of the range comes up.
    TEST_COVERAGE(UINT64_MAX, 1ULL << 48, passgen_random_u64(&random));

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
            (const uint8_t *) "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
            (const uint8_t *) "passgen") != NULL);
    CHECK_BEGINNING(&random);
    CHECK_REST(&random);

    // seek back to beginning, same values
    assert(passgen_random_chacha20_seek(&random, 0));
    CHECK_BEGINNING(&random);
    CHECK_REST(&random);

    // seek to middle of stream, only latter values
    assert(passgen_random_chacha20_seek(&random, 7));
    CHECK_REST(&random);

    passgen_random_close(&random);

    // any other source is left as it is.
    passgen_random other, same;
    assert(passgen_random_xorshift_open(&other, 1234));
    assert(passgen_random_xorshift_open(&same, 1234));
    assert_eq(passgen_random_u64(&other), passgen_random_u64(&same));
    assert(!passgen_random_chacha20_seek(&other, 0));
    assert_eq(passgen_random_u64(&other), passgen_random_u64(&same));
    passgen_random_close(&other);
    passgen_random_close(&same);

    return test_ok;
}

#undef CHECK_BEGINNING
#undef CHECK_REST

test_result test_random_chacha20(void) {
    passgen_random random;

    assert(
        passgen_random_chacha20_open(
            &random,
            (const uint8_t *) "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
            (const uint8_t *) "passgen") != NULL);
    assert(passgen_random_u8(&random) == 180);
    assert(passgen_random_u16(&random) == 43073);
    assert(passgen_random_u32(&random) == 1604870485);
    assert(passgen_random_u64(&random) == 5567227278517054982ULL);
    passgen_random_close(&random);

    assert(
        passgen_random_chacha20_open(
            &random,
            (const uint8_t *) "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb",
            (const uint8_t *) "passgen") != NULL);
    assert(passgen_random_u8(&random) == 14);
    assert(passgen_random_u16(&random) == 19281);
    assert(passgen_random_u32(&random) == 2608689089);
    assert(passgen_random_u64(&random) == 12744604773769097796ULL);
    passgen_random_close(&random);

    return test_ok;
}

// A cost which is quick, for the tests which do not check the levels.
static const passgen_master_cost small_cost = {.memory = 8, .passes = 1};

// Reads the first 64 bit number from the source for a master key, domain and
// user.
static uint64_t master_first(
    const passgen_master *master,
    const char *domain,
    const char *user) {
    passgen_random random;
    if(!passgen_master_random_open(&random, master, domain, user)) {
        return 0;
    }
    uint64_t number = passgen_random_u64(&random);
    passgen_random_close(&random);
    return number;
}

/// The levels have fixed costs, and only those of version 1 exist.
test_result test_random_master_level(void) {
    passgen_master_cost cost;
    assert(passgen_master_level(1, 1, &cost));
    assert_eq(cost.memory, 64 * 1024);
    assert_eq(cost.passes, 3);
    assert(passgen_master_level(1, 2, &cost));
    assert_eq(cost.memory, 256 * 1024);
    assert_eq(cost.passes, 3);
    assert(passgen_master_level(1, 3, &cost));
    assert_eq(cost.memory, 1024 * 1024);
    assert_eq(cost.passes, 3);

    assert(!passgen_master_level(1, 0, &cost));
    assert(!passgen_master_level(1, 4, &cost));
    assert(!passgen_master_level(0, 1, &cost));
    assert(!passgen_master_level(2, 1, &cost));

    passgen_master master;
    assert_eq(
        PASSGEN_ERROR_INVALID_ARGUMENT,
        passgen_master_derive(&master, 1, 0, "a", NULL, 0));
    assert_eq(
        PASSGEN_ERROR_INVALID_ARGUMENT,
        passgen_master_derive(&master, 2, 1, "a", NULL, 0));

    return test_ok;
}

/// Every input of a master key and of the source for a domain and a user
/// matters, and nothing else does. What the outputs are is checked by the
/// test vectors.
test_result test_random_master(void) {
    passgen_master master, other;
    assert_eq(
        PASSGEN_OK,
        passgen_master_derive_cost(&master, 1, &small_cost, "a", NULL, 0));
    assert_eq(master.version, 1);
    uint64_t reference = master_first(&master, "domain", "user");

    // it is the same every time, and an empty seed is the same as none.
    const uint8_t seed[PASSGEN_MASTER_SEED_MAX + 1] = {1, 2, 3};
    assert_eq(
        PASSGEN_OK,
        passgen_master_derive_cost(&other, 1, &small_cost, "a", seed, 0));
    assert(0 == memcmp(master.key, other.key, sizeof(master.key)));

    // the passphrase, the seed and the cost matter.
    assert_eq(
        PASSGEN_OK,
        passgen_master_derive_cost(&other, 1, &small_cost, "b", NULL, 0));
    assert(0 != memcmp(master.key, other.key, sizeof(master.key)));
    assert_eq(
        PASSGEN_OK,
        passgen_master_derive_cost(&other, 1, &small_cost, "a", seed, 1));
    assert(0 != memcmp(master.key, other.key, sizeof(master.key)));
    const passgen_master_cost more = {.memory = 8, .passes = 2};
    assert_eq(
        PASSGEN_OK,
        passgen_master_derive_cost(&other, 1, &more, "a", NULL, 0));
    assert(0 != memcmp(master.key, other.key, sizeof(master.key)));

    // the domain and the user matter, also where one ends and the other
    // starts, and no domain or user is the same as an empty one.
    assert_eq(master_first(&master, "domain", "user"), reference);
    assert(master_first(&master, "domain", "users") != reference);
    assert(master_first(&master, "domains", "user") != reference);
    assert(
        master_first(&master, "ab", "c") != master_first(&master, "a", "bc"));
    assert(master_first(&master, "x", "") != master_first(&master, "", "x"));
    assert_eq(master_first(&master, NULL, NULL), master_first(&master, "", ""));

    // the hash is not what the source gives out.
    uint8_t hash[PASSGEN_MASTER_HASH_SIZE], again[PASSGEN_MASTER_HASH_SIZE];
    passgen_master_hash(&master, hash);
    passgen_master_hash(&master, again);
    assert(0 == memcmp(hash, again, sizeof(hash)));
    passgen_master_hash(&other, again);
    assert(0 != memcmp(hash, again, sizeof(hash)));

    // what is not valid.
    assert_eq(
        PASSGEN_ERROR_INVALID_ARGUMENT,
        passgen_master_derive_cost(
            &other,
            1,
            &small_cost,
            "a",
            seed,
            PASSGEN_MASTER_SEED_MAX + 1));
    assert_eq(
        PASSGEN_OK,
        passgen_master_derive_cost(
            &other,
            1,
            &small_cost,
            "a",
            seed,
            PASSGEN_MASTER_SEED_MAX));
    const passgen_master_cost too_little = {.memory = 7, .passes = 1};
    const passgen_master_cost no_passes = {.memory = 8, .passes = 0};
    assert_eq(
        PASSGEN_ERROR_INVALID_ARGUMENT,
        passgen_master_derive_cost(&other, 1, &too_little, "a", NULL, 0));
    assert_eq(
        PASSGEN_ERROR_INVALID_ARGUMENT,
        passgen_master_derive_cost(&other, 1, &no_passes, "a", NULL, 0));
    assert_eq(
        PASSGEN_ERROR_INVALID_ARGUMENT,
        passgen_master_derive_cost(&other, 2, &small_cost, "a", NULL, 0));

    // wiping it leaves nothing.
    passgen_master_wipe(&master);
    const uint8_t zeroes[PASSGEN_MASTER_KEY_SIZE] = {0};
    assert(0 == memcmp(master.key, zeroes, sizeof(zeroes)));
    passgen_master_wipe(&other);

    return test_ok;
}

#endif

/// Running out of memory at any point while opening a source of randomness
/// fails, and leaves nothing allocated. Once there is enough memory, it is
/// the same source as always.
test_result test_random_out_of_memory(void) {
    const char *descs[] = {
        "zero",
        "xorshift:1234",
        "system",
#ifndef _WIN32
        "file:/dev/zero",
#endif
#ifdef PASSGEN_MONOCYPHER
        ("chacha20:" KEY ":0123456789abcdef"),
#endif
    };

    for(size_t i = 0; i < sizeof(descs) / sizeof(descs[0]); i++) {
        // what it is when nothing fails, for comparing.
        passgen_random reference;
        assert(passgen_random_open(&reference, descs[i]));
        uint8_t expected[64];
        passgen_random_read(&reference, expected, sizeof(expected));
        passgen_random_close(&reference);
        bool deterministic = 0 != strcmp(descs[i], "system");

        // opening it in place, and allocating it.
        for(int allocate = 0; allocate < 2; allocate++) {
            bool done = false;
            for(size_t fail_at = 0; !done; fail_at++) {
                test_allocator allocator;
                test_allocator_install(&allocator, fail_at);

                passgen_random in_place;
                passgen_random *random =
                    passgen_random_open(allocate ? NULL : &in_place, descs[i]);

                if(random) {
                    done = true;
                    assert(allocate || random == &in_place);
                    uint8_t output[64];
                    passgen_random_read(random, output, sizeof(output));
                    assert(!passgen_random_failed(random));
                    if(deterministic) {
                        assert(0 == memcmp(output, expected, sizeof(output)));
                    }

                    if(allocate) {
                        passgen_random_free(random);
                    } else {
                        passgen_random_close(random);
                    }
                }

                assert(test_allocator_clean(&allocator));
                test_allocator_remove();
            }
        }
    }

#ifdef PASSGEN_MONOCYPHER
    // deriving a key from a master passphrase allocates the memory for it,
    // which is kept small here so that this is quick.
    passgen_master master;
    assert_eq(
        PASSGEN_OK,
        passgen_master_derive_cost(&master, 1, &small_cost, "master", NULL, 0));
    uint64_t expected = master_first(&master, "example.com", NULL);
    passgen_master_wipe(&master);

    bool done = false;
    for(size_t fail_at = 0; !done; fail_at++) {
        test_allocator allocator;
        test_allocator_install(&allocator, fail_at);

        passgen_status status = passgen_master_derive_cost(
            &master,
            1,
            &small_cost,
            "master",
            NULL,
            0);
        if(status == PASSGEN_OK) {
            passgen_random *random =
                passgen_master_random_open(NULL, &master, "example.com", NULL);
            if(random) {
                done = true;
                assert_eq(passgen_random_u64(random), expected);
                passgen_random_free(random);
            }
            passgen_master_wipe(&master);
        } else {
            assert_eq(status, PASSGEN_ERROR_OUT_OF_MEMORY);
        }

        assert(test_allocator_clean(&allocator));
        test_allocator_remove();
    }
#endif

    return test_ok;
}

/// Descriptions are parsed into what they describe, and anything else is not
/// valid.
test_result test_random_desc_parse(void) {
    passgen_random_desc desc;

    assert(passgen_random_desc_parse(&desc, "zero"));
    assert_eq(desc.kind, PASSGEN_RANDOM_DESC_ZERO);
    assert(passgen_random_desc_parse(&desc, "system"));
    assert_eq(desc.kind, PASSGEN_RANDOM_DESC_SYSTEM);
    assert(!passgen_random_desc_parse(&desc, "systems"));
    assert(!passgen_random_desc_parse(&desc, "zero:"));
    assert(!passgen_random_desc_parse(&desc, ""));
    assert(!passgen_random_desc_parse(&desc, "unknown"));

    // the path is whatever follows, it is only checked when opening it.
    const char *file = "file:/dev/zero";
    assert(passgen_random_desc_parse(&desc, file));
    assert_eq(desc.kind, PASSGEN_RANDOM_DESC_FILE);
    assert(desc.data.file.path == &file[5]);
    assert(passgen_random_desc_parse(&desc, "file:"));
    assert_streq(desc.data.file.path, "");

    // seeds are digits only, and any 64 bit number other than zero.
    assert(passgen_random_desc_parse(&desc, "xorshift:1"));
    assert_eq(desc.kind, PASSGEN_RANDOM_DESC_XORSHIFT);
    assert_eq(desc.data.xorshift.seed, 1);
    assert(passgen_random_desc_parse(&desc, "xorshift:18446744073709551615"));
    assert_eq(desc.data.xorshift.seed, UINT64_MAX);
    assert(!passgen_random_desc_parse(&desc, "xorshift:18446744073709551616"));
    assert(!passgen_random_desc_parse(&desc, "xorshift:0"));
    assert(!passgen_random_desc_parse(&desc, "xorshift:"));
    assert(!passgen_random_desc_parse(&desc, "xorshift:+1"));
    assert(!passgen_random_desc_parse(&desc, "xorshift: 1"));
    assert(!passgen_random_desc_parse(&desc, "xorshift:1a"));

    return test_ok;
}

#ifdef PASSGEN_MONOCYPHER
/// ChaCha20 keys and IVs are in hex, and the master passphrase, domain and
/// token are separated by colons.
test_result test_random_desc_parse_chacha20(void) {
    passgen_random_desc desc;

    // the key is exactly 64 hex digits, and without an IV, the default one is
    // used.
    assert(passgen_random_desc_parse(&desc, "chacha20:" KEY));
    assert_eq(desc.kind, PASSGEN_RANDOM_DESC_CHACHA20);
    uint8_t key[32];
    for(size_t i = 0; i < sizeof(key); i++) {
        key[i] = (uint8_t) i;
    }
    assert(0 == memcmp(desc.data.chacha20.key, key, sizeof(key)));
    assert(0 == memcmp(desc.data.chacha20.iv, "passgen", 8));

    // the IV is exactly 16 hex digits, and both cases of them work.
    assert(
        passgen_random_desc_parse(&desc, "chacha20:" KEY ":0123456789ABCDEF"));
    const uint8_t iv[8] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef};
    assert(0 == memcmp(desc.data.chacha20.key, key, sizeof(key)));
    assert(0 == memcmp(desc.data.chacha20.iv, iv, sizeof(iv)));

    // keys and IVs which are not exactly as long, or not hex, used to be
    // taken as bytes, padded or cut off. they are not valid.
    const char *invalid[] = {
        "chacha20:",
        "chacha20:key",
        "chacha20:key:iv",
        // one digit short, one too many, not hex
        "chacha20:"
        "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1",
        ("chacha20:" KEY "0"),
        "chacha20:"
        "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1g",
        ("chacha20:" KEY ":"),
        ("chacha20:" KEY ":012345678abcdef"),
        ("chacha20:" KEY ":0123456789abcdef0"),
        ("chacha20:" KEY ":0123456789abcdeg"),
        ("chacha20:" KEY ":0123456789abcdef:"),
        ("chacha20: " KEY),
        "chacha20:"
        "0x0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f",
    };
    for(size_t i = 0; i < sizeof(invalid) / sizeof(invalid[0]); i++) {
        assert(!passgen_random_desc_parse(&desc, invalid[i]));
    }

    return test_ok;
}

/// Master passphrases are described by all of their fields, separated by
/// colons, which are escaped in the fields.
test_result test_random_desc_parse_master(void) {
    passgen_random_desc desc;

    assert(
        passgen_random_desc_parse(&desc, "master:1:2:pass:0aFF:domain:user"));
    assert_eq(desc.kind, PASSGEN_RANDOM_DESC_MASTER);
    assert_eq(desc.data.master.version, 1);
    assert_eq(desc.data.master.level, 2);
    assert_streq(desc.data.master.passphrase, "pass");
    assert_eq(desc.data.master.seed_size, 2);
    assert_eq(desc.data.master.seed[0], 0x0a);
    assert_eq(desc.data.master.seed[1], 0xff);
    assert_streq(desc.data.master.domain, "domain");
    assert_streq(desc.data.master.user, "user");
    assert(desc.data.master.cost == NULL);

    // all but the version, the level and the passphrase can be empty, and so
    // can the passphrase.
    assert(passgen_random_desc_parse(&desc, "master:1:1::::"));
    assert(!passgen_random_desc_parse(&desc, "master:1:1:::::"));
    assert_streq(desc.data.master.passphrase, "");
    assert_eq(desc.data.master.seed_size, 0);
    assert_streq(desc.data.master.domain, "");
    assert_streq(desc.data.master.user, "");

    // colons and backslashes are escaped.
    assert(passgen_random_desc_parse(
        &desc,
        "master:1:3:a\\:b\\\\::example.com\\:8443:\\\\"));
    assert_eq(desc.data.master.level, 3);
    assert_streq(desc.data.master.passphrase, "a:b\\");
    assert_streq(desc.data.master.domain, "example.com:8443");
    assert_streq(desc.data.master.user, "\\");

    const char *invalid[] = {
        "master:",
        "master:1:2:pass:",
        "master:1:2:pass::domain",
        // one field too many
        "master:1:2:pass::domain:user:",
        "master:1:2:pass::domain:user:x",
        // unknown versions and levels
        "master:0:2:pass::domain:user",
        "master:2:2:pass::domain:user",
        "master:1:0:pass::domain:user",
        "master:1:4:pass::domain:user",
        "master::2:pass::domain:user",
        "master:1::pass::domain:user",
        "master:01:2:pass::domain:user",
        "master:1:+2:pass::domain:user",
        // seeds which are not whole bytes in hex
        "master:1:2:pass:0:domain:user",
        "master:1:2:pass:0g:domain:user",
        "master:1:2:pass:0x00:domain:user",
        // other backslashes
        "master:1:2:pa\\ss::domain:user",
        "master:1:2:pass::domain:user\\",
    };
    for(size_t i = 0; i < sizeof(invalid) / sizeof(invalid[0]); i++) {
        assert(!passgen_random_desc_parse(&desc, invalid[i]));
    }

    // the passphrase, domain and user are at most 128 bytes each, which an
    // escape counts as one of, and the seed is at most 64 bytes.
    char string[256];
    strcpy(string, "master:1:2:");
    size_t start = strlen(string);
    memset(&string[start], 'x', 127);
    strcpy(&string[start + 127], "\\:::domain:user");
    assert(passgen_random_desc_parse(&desc, string));
    assert_eq(strlen(desc.data.master.passphrase), 128);
    memset(&string[start], 'x', 128);
    strcpy(&string[start + 128], "\\:::domain:user");
    assert(!passgen_random_desc_parse(&desc, string));

    strcpy(string, "master:1:2:pass:");
    start = strlen(string);
    memset(&string[start], 'a', 128);
    strcpy(&string[start + 128], ":domain:user");
    assert(passgen_random_desc_parse(&desc, string));
    assert_eq(desc.data.master.seed_size, 64);
    memset(&string[start], 'a', 130);
    strcpy(&string[start + 130], ":domain:user");
    assert(!passgen_random_desc_parse(&desc, string));

    return test_ok;
}

/// Whether a description may hold a secret only depends on its kind, so it
/// works for descriptions which are not valid, which is when it is needed (to
/// not print the secret in an error).
test_result test_random_desc_secret(void) {
    const char *secret[] = {
        ("chacha20:" KEY),
        "chacha20:secretkey",
        "master:1:2:pass::example.com:user",
        "master:",
        // not a known kind, but maybe a mistyped one
        "chacha:secretkey",
        "chacha20",
        "",
        "zero:",
        "systemx",
    };
    for(size_t i = 0; i < sizeof(secret) / sizeof(secret[0]); i++) {
        assert(passgen_random_desc_secret(secret[i]));
    }

    const char *public[] = {
        "zero",
        "system",
        "file:/dev/urandom",
        "file:",
        "xorshift:1234",
        "xorshift:0",
        "xorshift:abc",
    };
    for(size_t i = 0; i < sizeof(public) / sizeof(public[0]); i++) {
        assert(!passgen_random_desc_secret(public[i]));
    }

    return test_ok;
}

/// What is parsed opens the same source as opening the description does.
test_result test_random_desc_open(void) {
    const char *descs[] = {
        "zero",
        "xorshift:1234",
#ifdef PASSGEN_MONOCYPHER
        ("chacha20:" KEY ":0123456789abcdef"),
#endif
    };

    for(size_t i = 0; i < sizeof(descs) / sizeof(descs[0]); i++) {
        passgen_random_desc desc;
        assert(passgen_random_desc_parse(&desc, descs[i]));
        passgen_random *parsed = passgen_random_desc_open(NULL, &desc);
        assert(parsed);
        passgen_random *opened = passgen_random_open(NULL, descs[i]);
        assert(opened);

        uint8_t expected[64], output[64];
        passgen_random_read(opened, expected, sizeof(expected));
        passgen_random_read(parsed, output, sizeof(output));
        assert(0 == memcmp(output, expected, sizeof(output)));

        passgen_random_free(parsed);
        passgen_random_free(opened);
    }

    // keys are derived with the parameters which it has. the usual ones
    // are slow, and checked by the test vectors.
    passgen_random_desc desc;
    assert(
        passgen_random_desc_parse(&desc, "master:1:3:pass:00:example.com:1"));
    desc.data.master.cost = &small_cost;
    passgen_random *parsed = passgen_random_desc_open(NULL, &desc);
    passgen_master master;
    const uint8_t seed[1] = {0};
    assert_eq(
        PASSGEN_OK,
        passgen_master_derive_cost(&master, 1, &small_cost, "pass", seed, 1));
    assert(parsed);
    assert_eq(
        passgen_random_u64(parsed),
        master_first(&master, "example.com", "1"));
    passgen_random_free(parsed);
    passgen_master_wipe(&master);

    return test_ok;
}
#endif

#ifndef _WIN32
// Exit code of the child process when it aborted.
#define ABORTED_EXIT 42

// Turns aborting into exiting, so that it does not go through what the
// system does for a crash, which is slow, and hangs under the leaks tool.
static void exit_on_abort(int signal) {
    (void) signal;
    _exit(ABORTED_EXIT);
}

/// Asking for a number smaller than zero is a mistake of the caller, which
/// would loop forever. It aborts in every build, which is checked in a child
/// process.
test_result test_random_max_zero(void) {
    for(int bits = 8; bits <= 64; bits *= 2) {
        pid_t child = fork();
        assert(child >= 0);
        if(child == 0) {
            // what it prints is expected.
            freopen("/dev/null", "w", stderr);
            signal(SIGABRT, exit_on_abort);
            passgen_random random;
            passgen_random_zero_open(&random);
            switch(bits) {
                case 8:
                    passgen_random_u8_max(&random, 0);
                    break;
                case 16:
                    passgen_random_u16_max(&random, 0);
                    break;
                case 32:
                    passgen_random_u32_max(&random, 0);
                    break;
                default:
                    passgen_random_u64_max(&random, 0);
                    break;
            }
            _exit(0);
        }

        int status = 0;
        assert(child == waitpid(child, &status, 0));
        assert(WIFEXITED(status));
        assert_eq(WEXITSTATUS(status), ABORTED_EXIT);
    }

    return test_ok;
}
#endif
