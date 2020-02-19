#include "passgen/utf8.h"
#include "tests.h"

test_result test_utf8_decode_simple(void) {
    // test that we can decode a simple UTF-8 string.

    // ü😂µ
    const uint8_t input[] = {
        0xc3, 0xbc, 0xf0, 0x9f, 0x98, 0x82, 0xc2, 0xb5, 0x0a
    };

    uint32_t output[100];

    size_t in_pos = 0;
    size_t out_pos = 0;

    int ret = utf8_decode(
            output, sizeof(output) / sizeof(output[0]), &out_pos,
            input, sizeof(input), &in_pos);

    assert(ret == 0);
    assert(in_pos == sizeof(input));
    assert(output[0] == 0xFC);
    assert(output[1] == 0x1F602);
    assert(output[2] == 0xB5);
    assert(output[3] == 0x0A);
    assert(out_pos == 4);

    return test_ok;
}

test_result test_utf8_decode_short_output(void) {
    // test that we can decode a simple UTF-8 string, if there is not enough
    // space in the output array, by restarting the decoding.

    // ü😂µ
    const uint8_t input[] = {
        0xc3, 0xbc, 0xf0, 0x9f, 0x98, 0x82, 0xc2, 0xb5, 0x0a
    };

    uint32_t output[3];

    size_t in_pos = 0;
    size_t out_pos = 0;

    int ret = utf8_decode(
            output, sizeof(output) / sizeof(output[0]), &out_pos,
            input, sizeof(input), &in_pos);

    // on first run, it should only decode the first three characters. it
    // should return larger than zero because there is still content to be
    // decoded.
    assert(ret > 0);
    assert(in_pos == sizeof(input) - 1);
    assert(output[0] == 0xFC);
    assert(output[1] == 0x1F602);
    assert(output[2] == 0xB5);
    assert(out_pos == 3);

    out_pos = 0;

    ret = utf8_decode(
            output, sizeof(output) / sizeof(output[0]), &out_pos,
            input, sizeof(input), &in_pos);

    // on second run, the final character is decoded.
    assert(ret == 0);
    assert(in_pos == sizeof(input));
    assert(output[0] == 0x0A);
    assert(out_pos == 1);

    return test_ok;
}
