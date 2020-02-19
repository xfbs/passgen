#pragma once
#include <stddef.h>
#include <stdint.h>

/* decodes a UTF-8 character sequence into an output array.
 * out should point to an array with out_len items, this is where the decoded
 * charpoints are stored. Writing starts at *out_pos, and *out_pos is incremented
 * every time something is written.
 * in should point to UTF-8 data with in_len bytes. Reading starts at *in_pos
 * and this is updated to reflect the current position of the decoding.
 * Returns 0 if all input data has been successfully decoded, a positive integer
 * if there is still data left to decode (but the output buffer is full) and a
 * negative integer on error (such as when there is an illegal sequence
 * encountered or there input is incomplete).
 */
int utf8_decode(uint32_t *out, size_t out_len, size_t *out_pos, const uint8_t *in, size_t in_len, size_t *in_pos);
