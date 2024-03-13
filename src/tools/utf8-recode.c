#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "passgen/assert.h"
#include "passgen/util/utf8.h"

int main(int argc, char *argv[]) {
    size_t input_len = 128;
    uint8_t input[input_len];

    size_t input_start = 0;
    size_t input_end = 0;

    size_t decoded_len = 128;
    uint32_t decoded[decoded_len];
    size_t decoded_total = 0;

    size_t output_len = 128;
    uint8_t output[output_len];

    int ret;

    (void) argc;
    (void) argv;

    while(true) {
        if(ferror(stdin)) {
            goto error;
        }

        if(feof(stdin)) {
            break;
        }

        // read a chunk of data into the input buffer
        input_end += fread(
            input + input_end,
            sizeof(input[0]),
            sizeof(input) - input_end,
            stdin);

        printf("read %zu bytes\n", input_end);

        uint32_t *decoded_pos = &decoded[0];
        const uint8_t *input_pos = &input[0];

        ret = passgen_utf8_decode(
            &decoded_pos,
            decoded_len,
            NULL,
            &input_pos,
            input_end);

        decoded_total += decoded_pos - &decoded[0];
        passgen_assert(ret == PASSGEN_UTF8_SUCCESS);

        printf("ret is %i\n", ret);

        printf("read %zu codepoints\n", decoded_total);

        decoded_pos = 0;
        input_start = 0;
        input_end = 0;
    }

    return EXIT_SUCCESS;

error:
    return EXIT_FAILURE;
}
