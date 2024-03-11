#include "passgen/util/utf8.h"
#include "tests.h"

// test that we can decode an empty string.
test_result test_utf8_can_decode_empty(void) {
    size_t input_len = 0;
    const uint8_t input[input_len + 1];
    size_t output_len = 0;
    uint32_t output[output_len + 1];
    const uint8_t *input_pos = &input[0];
    uint32_t *output_pos = &output[0];

    int ret = passgen_utf8_decode(
        &output_pos,
        output_len,
        NULL,
        &input_pos,
        input_len);

    assert(ret == PASSGEN_UTF8_SUCCESS);
    assert(output_pos == &output[0]);
    assert(input_pos == &input[0]);

    return test_ok;
}

// test that we can decode a simple utf-8 sequence.
test_result test_utf8_can_decode_simple(void) {
    // ÃÂ¼Ã°ÂÂÂÃÂµ
    const uint8_t input[] =
        {0xc3, 0xbc, 0xf0, 0x9f, 0x98, 0x82, 0xc2, 0xb5, 0x0a};
    size_t input_len = sizeof(input);

    size_t output_len = 100;
    uint32_t output[output_len];

    const uint8_t *input_pos = &input[0];
    uint32_t *output_pos = &output[0];

    int ret = passgen_utf8_decode(
        &output_pos,
        output_len,
        NULL,
        &input_pos,
        input_len);

    // succesful return
    assert(ret == PASSGEN_UTF8_SUCCESS);

    // parsed all data
    assert(input_pos == &input[input_len]);
    assert(output_pos == &output[4]);

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
    size_t input_len = sizeof(input);

    size_t output_len = 100;
    uint32_t output[output_len];
    uint8_t widths[output_len];

    const uint8_t *input_pos = &input[0];
    uint32_t *output_pos = &output[0];

    int ret = passgen_utf8_decode(
        &output_pos,
        output_len,
        &widths[0],
        &input_pos,
        input_len);

    // successful return
    assert(ret == PASSGEN_UTF8_SUCCESS);

    // processed all input data
    assert(input_pos == &input[input_len]);
    assert(output_pos == &output[4]);

    // parsed characters properly
    assert(output[0] == 0xFC);
    assert(output[1] == 0x1F602);
    assert(output[2] == 0xB5);
    assert(output[3] == 0x0A);

    // parsed widths correctly
    assert(widths[0] == 2);
    assert(widths[1] == 4);
    assert(widths[2] == 2);
    assert(widths[3] == 1);

    return test_ok;
}

// test that we can decode a simple UTF-8 string, if there is not enough
// space in the output array, by restarting the decoding.
test_result test_utf8_decode_short_output(void) {
    // ÃÂ¼Ã°ÂÂÂÃÂµ
    const uint8_t input[] =
        {0xc3, 0xbc, 0xf0, 0x9f, 0x98, 0x82, 0xc2, 0xb5, 0x0a};
    size_t input_len = sizeof(input);

    size_t output_len = 3;
    uint32_t output[output_len];

    const uint8_t *input_pos = &input[0];
    uint32_t *output_pos = &output[0];

    int ret = passgen_utf8_decode(
        &output_pos,
        output_len,
        NULL,
        &input_pos,
        input_len);

    // on first run, it should only decode the first three characters. it
    // should return larger than zero because there is still content to be
    // decoded.
    assert(ret == PASSGEN_UTF8_OUTPUT_SIZE);
    assert(input_pos == &input[8]);
    assert(output_pos == &output[output_len]);
    assert(output[0] == 0xFC);
    assert(output[1] == 0x1F602);
    assert(output[2] == 0xB5);

    // reset where the current output pointer is at
    output_pos = &output[0];

    // continue parsing the rest of the utf-8 sequence.
    ret = passgen_utf8_decode(
        &output_pos,
        output_len,
        NULL,
        &input_pos,
        input_len - 8);

    // on second run, the final character is decoded.
    assert(ret == PASSGEN_UTF8_SUCCESS);
    assert(input_pos == &input[input_len]);
    assert(output_pos == &output[1]);
    assert(output[0] == 0x0A);

    return test_ok;
}

test_result test_utf8_encode_simple(void) {
    // test that we can decode a simple UTF-8 string.

    // ÃÂ¼Ã°ÂÂÂÃÂµ
    const uint8_t expected[] =
        {0xc3, 0xbc, 0xf0, 0x9f, 0x98, 0x82, 0xc2, 0xb5, 0x0a};

    const uint32_t input[] = {0xFC, 0x1F602, 0xB5, 0x0A};

    size_t output_len = 15;
    uint8_t output[output_len];

    size_t in_pos = 0;
    size_t out_pos = 0;

    int ret = passgen_utf8_encode(
        output,
        output_len,
        &out_pos,
        input,
        sizeof(input) / sizeof(input[0]),
        &in_pos);

    assert(ret == PASSGEN_UTF8_SUCCESS);
    assert(in_pos == (sizeof(input) / sizeof(input[0])));
    assert(out_pos == (sizeof(expected) / sizeof(expected[0])));
    for(size_t i = 0; i < out_pos; i++) {
        assert(output[i] == expected[i]);
    }

    return test_ok;
}
