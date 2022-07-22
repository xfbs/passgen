#include "passgen/utf8.h"
#include <utf8proc.h>

int passgen_utf8_decode(
    uint32_t *output,
    size_t output_len,
    size_t *output_pos,
    uint8_t *output_widths,
    const uint8_t *input,
    size_t input_len,
    size_t *input_pos) {
    utf8proc_ssize_t n;

    while((output_len > *output_pos) && (input_len > *input_pos)) {
        n = utf8proc_iterate(
            input + *input_pos,
            input_len - *input_pos,
            (utf8proc_int32_t *) (output) + *output_pos);

        if(n < 0) {
            return n;
        }

        // save offset
        if(output_widths) {
            output_widths[*output_pos] = n;
        }

        *input_pos += n;
        *output_pos += 1;
    }

    return input_len - *input_pos;
}

int passgen_utf8_encode(
    uint8_t *out,
    size_t out_len,
    size_t *out_pos,
    const uint32_t *in,
    size_t in_len,
    size_t *in_pos) {
    utf8proc_ssize_t n;

    // TODO: fixeme. what?
    (void) in_len;

    while((out_len - *out_pos) > 0 &&
          (n = utf8proc_encode_char(in[*in_pos], &out[*out_pos])) > 0) {
        *in_pos += 1;
        *out_pos += n;
    }

    return n;
}
