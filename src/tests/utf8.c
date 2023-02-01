#include "passgen/util/utf8.h"
#include "tests.h"

// test that we can decode an empty string.
test_result test_utf8_can_decode_empty(void) {
    size_t input_size = 0;
    const uint8_t input[input_size];
    size_t output_size = 0;
    uint32_t output[output_size];
    size_t input_pos = 0;
    size_t output_pos = 0;

    int ret = passgen_utf8_decode(
        &output[0],
        output_size,
        &output_pos,
        NULL,
        &input[0],
        input_size,
        &input_pos);

    assert(ret == 0);
    assert(output_pos == 0);
    assert(input_pos == 0);

    return test_ok;
}

// test that we can decode an empty string with character widths.
test_result test_utf8_can_decode_empty_widths(void) {
    size_t input_size = 0;
    const uint8_t input[input_size];
    size_t output_size = 1;
    uint32_t output[output_size];
    uint8_t widths[output_size];
    size_t input_pos = 0;
    size_t output_pos = 0;

    int ret = passgen_utf8_decode(
        &output[0],
        output_size,
        &output_pos,
        &widths[0],
        &input[0],
        input_size,
        &input_pos);

    assert(ret == 0);
    assert(output_pos == 0);
    assert(input_pos == 0);

    return test_ok;
}

// test that we can decode a simple utf-8 sequence.
test_result test_utf8_can_decode_simple(void) {
    // ÃÂ¼Ã°ÂÂÂÃÂµ
    const uint8_t input[] =
        {0xc3, 0xbc, 0xf0, 0x9f, 0x98, 0x82, 0xc2, 0xb5, 0x0a};
    size_t input_size = sizeof(input);

    size_t output_size = 100;
    uint32_t output[output_size];

    size_t input_pos = 0;
    size_t output_pos = 0;

    int ret = passgen_utf8_decode(
        output,
        output_size,
        &output_pos,
        NULL,
        input,
        input_size,
        &input_pos);

    // succesful return
    assert(ret == 0);

    // parsed all data
    assert(input_pos == input_size);
    assert(output_pos == 4);

    // parsed right characters
    assert(output[0] == 0xFC);
    assert(output[1] == 0x1F602);
    assert(output[2] == 0xB5);
    assert(output[3] == 0x0A);

    return test_ok;
}

// test that we can decode a simple utf-8 sequence with character widths.
test_result test_utf8_can_decode_simple_widths(void) {
    // ÃÂ¼Ã°ÂÂÂÃÂµ
    const uint8_t input[] =
        {0xc3, 0xbc, 0xf0, 0x9f, 0x98, 0x82, 0xc2, 0xb5, 0x0a};
    size_t input_size = sizeof(input);

    size_t output_size = 100;
    uint32_t output[output_size];
    uint8_t widths[output_size];

    size_t input_pos = 0;
    size_t output_pos = 0;

    int ret = passgen_utf8_decode(
        output,
        output_size,
        &output_pos,
        &widths[0],
        input,
        input_size,
        &input_pos);

    // successful return
    assert(ret == 0);

    // processed all input data
    assert(input_pos == input_size);
    assert(output_pos == 4);

    // parsed characters properly
    assert(output[0] == 0xFC);
    assert(widths[0] == 2);

    assert(output[1] == 0x1F602);
    assert(widths[1] == 4);

    assert(output[2] == 0xB5);
    assert(widths[2] == 2);

    assert(output[3] == 0x0A);
    assert(widths[3] == 1);

    return test_ok;
}

// test that we can decode a simple UTF-8 string, if there is not enough
// space in the output array, by restarting the decoding.
test_result test_utf8_decode_short_output(void) {
    // ÃÂ¼Ã°ÂÂÂÃÂµ
    const uint8_t input[] =
        {0xc3, 0xbc, 0xf0, 0x9f, 0x98, 0x82, 0xc2, 0xb5, 0x0a};
    size_t input_size = sizeof(input);

    size_t output_size = 3;
    uint32_t output[output_size];

    size_t input_pos = 0;
    size_t output_pos = 0;

    int ret = passgen_utf8_decode(
        output,
        output_size,
        &output_pos,
        NULL,
        input,
        input_size,
        &input_pos);

    // on first run, it should only decode the first three characters. it
    // should return larger than zero because there is still content to be
    // decoded.
    assert(ret > 0);
    assert(input_pos == input_size - 1);
    assert(output_pos == output_size);
    assert(output[0] == 0xFC);
    assert(output[1] == 0x1F602);
    assert(output[2] == 0xB5);

    // reset where the current output pointer is at
    output_pos = 0;

    // continue parsing the rest of the utf-8 sequence.
    ret = passgen_utf8_decode(
        output,
        output_size,
        &output_pos,
        NULL,
        input,
        input_size,
        &input_pos);

    // on second run, the final character is decoded.
    assert(ret == 0);
    assert(input_pos == input_size);
    assert(output_pos == 1);
    assert(output[0] == 0x0A);

    return test_ok;
}

test_result test_utf8_encode_simple(void) {
    // test that we can decode a simple UTF-8 string.

    // ÃÂ¼Ã°ÂÂÂÃÂµ
    const uint8_t expected[] =
        {0xc3, 0xbc, 0xf0, 0x9f, 0x98, 0x82, 0xc2, 0xb5, 0x0a};

    const uint32_t input[] = {0xFC, 0x1F602, 0xB5, 0x0A};

    uint8_t output[9];

    size_t in_pos = 0;
    size_t out_pos = 0;

    int ret = passgen_utf8_encode(
        output,
        sizeof(output) / sizeof(output[0]),
        &out_pos,
        input,
        sizeof(input) / sizeof(input[0]),
        &in_pos);

    assert(ret != 0);
    assert(in_pos == (sizeof(input) / sizeof(input[0])));
    assert(out_pos == (sizeof(expected) / sizeof(expected[0])));
    for(size_t i = 0; i < out_pos; i++) {
        assert(output[i] == expected[i]);
    }

    return test_ok;
}
