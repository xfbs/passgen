#include "passgen/util/siphash.h"
#include "tests.h"

test_result test_siphash64(void) {
    // test siphash64 (8 byte output)
    const char *key = "zppyfssftiwghlcy";
    struct {
        const char *input;
        uint8_t output[8];
    } testcases[] = {
        {.input = "input",
         .output = {0x6c, 0xef, 0x54, 0xb9, 0xcb, 0x8a, 0xe9, 0x43}},
        {.input = "test",
         .output = {0xf0, 0x32, 0x3b, 0x4f, 0x2d, 0x61, 0x5d, 0x99}},
        {.input = "passgen",
         .output = {0x45, 0x10, 0x48, 0x0e, 0x04, 0x58, 0xc0, 0xee}},
        {.input = "pattern",
         .output = {0x83, 0x99, 0xf6, 0xc6, 0x57, 0x7e, 0xdb, 0x7e}},
        {.input = "hash",
         .output = {0xbf, 0x27, 0xf4, 0x7b, 0xf8, 0xb4, 0xef, 0xbe}},
        {.input = "asam",
         .output = {0x9c, 0x7d, 0xae, 0x86, 0xdd, 0x97, 0x94, 0xe6}},
        {.input = "bsam",
         .output = {0xce, 0x87, 0x56, 0x58, 0xbb, 0x3c, 0x80, 0xe1}},
        {.input = "longjohn",
         .output = {0xbc, 0xfd, 0x5a, 0xf7, 0x43, 0xaf, 0x4a, 0x55}},
        {NULL}};

    for(size_t i = 0; testcases[i].input; i++) {
        uint8_t output[8];
        const char *input = testcases[i].input;
        assert_eq(
            0,
            passgen_siphash(input, strlen(input), key, output, sizeof(output)));
        for(size_t b = 0; b < sizeof(output); b++) {
            assert_eq(output[b], testcases[i].output[b]);
        }
    }

    return test_ok;
}

test_result test_siphash128(void) {
    // test siphash128 (16 byte output)
    const char *key = "zppyfssftiwghlcy";
    struct {
        const char *input;
        uint8_t output[16];
    } testcases[] = {
        {.input = "input",
         .output =
             {0xaa,
              0x06,
              0xb4,
              0x1b,
              0x1e,
              0x00,
              0xaa,
              0x80,
              0x07,
              0xdc,
              0xd5,
              0x7b,
              0x82,
              0x33,
              0x79,
              0x1a}},
        {.input = "test",
         .output =
             {0x9c,
              0x04,
              0x7e,
              0x22,
              0x1c,
              0x46,
              0x69,
              0xd6,
              0xd7,
              0xe1,
              0x73,
              0xa8,
              0x66,
              0xeb,
              0x93,
              0x2b}},
        {.input = "passgen",
         .output =
             {0xbf,
              0x62,
              0x3a,
              0xac,
              0x34,
              0x5a,
              0xd7,
              0x74,
              0xab,
              0x83,
              0xac,
              0xad,
              0x04,
              0xd4,
              0xa0,
              0xed}},
        {.input = "pattern",
         .output =
             {0x26,
              0xe5,
              0x55,
              0xd0,
              0xd1,
              0x90,
              0xbc,
              0x9e,
              0x8c,
              0x34,
              0x7f,
              0xe0,
              0x87,
              0xd6,
              0x87,
              0xdc}},
        {.input = "hash",
         .output =
             {0x34,
              0x70,
              0x1f,
              0x51,
              0xac,
              0xce,
              0x41,
              0x65,
              0x19,
              0x0f,
              0x1c,
              0x72,
              0x1b,
              0x84,
              0x45,
              0xd8}},
        {.input = "asam",
         .output =
             {0xd6,
              0x19,
              0x0d,
              0xfa,
              0xdc,
              0xfc,
              0xd2,
              0x5a,
              0xbf,
              0xa8,
              0xdc,
              0x78,
              0x41,
              0xb4,
              0xa9,
              0xd0}},
        {.input = "bsam",
         .output =
             {0xe3,
              0x70,
              0x02,
              0x94,
              0x4b,
              0xc4,
              0xeb,
              0xec,
              0x92,
              0x3e,
              0xee,
              0xa2,
              0xfb,
              0x18,
              0x5d,
              0xef}},
        {.input = "longjohn",
         .output =
             {0xdf,
              0xa7,
              0x8a,
              0x97,
              0xf0,
              0xdf,
              0x0e,
              0x62,
              0xc5,
              0x1b,
              0x8b,
              0x6b,
              0x79,
              0xda,
              0x3f,
              0xde}},
        {NULL}};

    for(size_t i = 0; testcases[i].input; i++) {
        uint8_t output[16];
        const char *input = testcases[i].input;
        assert_eq(
            0,
            passgen_siphash(input, strlen(input), key, output, sizeof(output)));
        for(size_t b = 0; b < sizeof(output); b++) {
            assert_eq(output[b], testcases[i].output[b]);
        }
    }

    return test_ok;
}
