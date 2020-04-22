#include "passgen/utf8.h"
#include <utf8proc.h>

int utf8_decode(
    uint32_t *out,
    size_t out_len,
    size_t *out_pos,
    const uint8_t *in,
    size_t in_len,
    size_t *in_pos) {
  utf8proc_ssize_t n;

  while((out_len - *out_pos) > 0 &&
        (n = utf8proc_iterate(
             in + *in_pos,
             in_len - *in_pos,
             (utf8proc_int32_t *) (out) + *out_pos)) > 0) {
    *out_pos += 1;
    *in_pos += n;
  }

  return n;
}

int utf8_encode(
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
