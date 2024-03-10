#include "passgen/util/utf8.h"
#include <utf8proc.h>

// Copyright (c) 2008-2009 Bjoern Hoehrmann <bjoern@hoehrmann.de>
// See http://bjoern.hoehrmann.de/utf-8/decoder/dfa/ for details.

#define UTF8_ACCEPT 0
#define UTF8_REJECT 1

static const uint8_t utf8d[] = {
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 00..1f
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 20..3f
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 40..5f
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 60..7f
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9, // 80..9f
  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7, // a0..bf
  8,8,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, // c0..df
  0xa,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x4,0x3,0x3, // e0..ef
  0xb,0x6,0x6,0x6,0x5,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8, // f0..ff
  0x0,0x1,0x2,0x3,0x5,0x8,0x7,0x1,0x1,0x1,0x4,0x6,0x1,0x1,0x1,0x1, // s0..s0
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,0,1,0,1,1,1,1,1,1, // s1..s2
  1,2,1,1,1,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1, // s3..s4
  1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,3,1,3,1,1,1,1,1,1, // s5..s6
  1,3,1,1,1,1,1,3,1,3,1,1,1,1,1,1,1,3,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // s7..s8
};

inline static uint32_t utf8_decode(uint32_t* state, uint32_t* codep, uint32_t byte) {
  uint32_t type = utf8d[byte];

  *codep = (*state != UTF8_ACCEPT) ?
    (byte & 0x3fu) | (*codep << 6) :
    (0xff >> type) & (byte);

  *state = utf8d[256 + *state*16 + type];
  return *state;
}

int passgen_utf8_decode(
    uint32_t **output,
    size_t output_len,
    uint8_t *output_widths,
    const uint8_t **input,
    size_t input_len) {

    uint32_t state = UTF8_ACCEPT;
    uint32_t codepoint = 0;

    size_t input_offset = 0;
    size_t output_offset = 0;
    size_t codepoint_width = 0;

    // iterate while we still have input and we still have outputs.
    while((input_offset < input_len) && (output_offset < output_len)) {
        uint32_t status = utf8_decode(&state, &codepoint, (*input)[input_offset]);

        if(status == UTF8_REJECT) {
            return PASSGEN_UTF8_INVALID_CHAR;
        }

        if(status == UTF8_ACCEPT) {
            (*output)[output_offset] = codepoint;

            if(output_widths) {
                output_widths[output_offset] = codepoint_width + 1;
            }

            codepoint_width = 0;
            codepoint = 0;
            output_offset += 1;
        } else {
            codepoint_width += 1;
        }

        input_offset += 1;
    }

    *output += output_offset;
    *input += input_offset;

    if(input_offset == input_len) {
        return PASSGEN_UTF8_SUCCESS;
    } else {
        return PASSGEN_UTF8_OUTPUT_SIZE;
    }
}

int passgen_utf8_encode(
    uint8_t *out,
    size_t out_len,
    size_t *out_pos,
    const uint32_t *in,
    size_t in_len,
    size_t *in_pos) {
    utf8proc_ssize_t n = 0;

    // TODO: fixeme. what?
    (void) in_len;

    while((out_len - *out_pos) > 0 &&
          (n = utf8proc_encode_char(in[*in_pos], &out[*out_pos])) > 0) {
        *in_pos += 1;
        *out_pos += n;
    }

    return n;
}

const char *passgen_utf8_error(int retval) {
    return utf8proc_errmsg(retval);
}
