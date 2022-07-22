#pragma once
#include <stddef.h>
#include <stdint.h>

/* decodes a UTF-8 character sequence into an output array.
 * out should point to an array with out_len items, this is where the decoded
 * charpoints are stored. Writing starts at *out_pos, and *out_pos is
 * incremented every time something is written. in should point to UTF-8 data
 * with in_len bytes. Reading starts at *in_pos and this is updated to reflect
 * the current position of the decoding. Returns 0 if all input data has been
 * successfully decoded, a positive integer if there is still data left to
 * decode (but the output buffer is full) and a negative integer on error (such
 * as when there is an illegal sequence encountered or there input is
 * incomplete).
 *
 * @param output The decoded unicode output. This must point to a uint32_t array
 *   of size `output_len`.
 * @param output_len The length of `output`, and if it is supplied, `output_widths`.
 * @param output_pos A pointer to a `size_t variable containing the current offset
 *   into the `output` array. The value of that variable should initially be zero.
 *   At the end, this variable indicates how many unicode codepoints were written
 *   into the output.
 * @param output_widths A pointer to an array of `uint8_t`, which will contain
 *   information of the byte width of every parsed unicode codepoint. If a `NULL`
 *   pointer is passed, this behaviour is disabled.
 * @param input The UTF-8 input.
 * @param input_len The length of the UTF-8 input sequence.
 * @param input_pos Pointer to a `size_t` containing the current offset into the
 *   input array. The value of this variable should initially be set to zero. After
 *   returning, it will indicate the number of processed bytes from the input.
 */
int passgen_utf8_decode(
    uint32_t *output,
    size_t output_len,
    size_t *output_pos,
    uint8_t *output_widths,
    const uint8_t *input,
    size_t input_len,
    size_t *input_pos);

/* encodes unicode characters into a UTF-8 character sequence.
 */
int passgen_utf8_encode(
    uint8_t *out,
    size_t out_len,
    size_t *out_pos,
    const uint32_t *in,
    size_t in_len,
    size_t *in_pos);
