/// @file utf8.h
/// @author Patrick M. Elsen <pelsen@xfbs.net>
/// @brief UTF-8 encoding and decoding utilities.
///
/// Internally, passgen has proper Unicode support (meaning that it works with UTF-32
/// codepoints). However, for transport and storage, UTF-8 is more common. Therefore,
/// these functions are needed to convert from UTF-8 to Unicode codepoints, and vice
/// versa.
///
/// These utilities are wrappers around the functionality provided by the incredible
/// [utf8proc](https://github.com/JuliaStrings/utf8proc) library of the Julia language.
#pragma once
#include <stddef.h>
#include <stdint.h>

/// Decodes a UTF-8 character sequence into an output array.
///
/// Decodes the UTF-8 character sequence in `input` (with length `input_len`) into
/// the Unicode codepoint array `output` (with length `output_len`). The number of
/// bytes processed from the input is written into the value pointed to by `input_pos`,
/// and the amount of codepoints written into `output` is written into the value
/// pointed to by `output_pos`. If `output_widths` is supplied, then for every decoded
/// codepoint, the byte width of the codepoint is written into it.
///
/// @param output The decoded unicode output. This must point to a uint32_t array
///   of size `output_len`.
/// @param output_len The length of `output`, and if it is supplied, `output_widths`.
/// @param output_pos A pointer to a `size_t variable containing the current offset
///   into the `output` array. The value of that variable should initially be zero.
///   At the end, this variable indicates how many unicode codepoints were written
///   into the output.
/// @param output_widths A pointer to an array of `uint8_t`, which will contain
///   information of the byte width of every parsed unicode codepoint. If a `NULL`
///   pointer is passed, this behaviour is disabled.
/// @param input The UTF-8 input.
/// @param input_len The length of the UTF-8 input sequence.
/// @param input_pos Pointer to a `size_t` containing the current offset into the
///   input array. The value of this variable should initially be set to zero. After
///   returning, it will indicate the number of processed bytes from the input.
/// @return 0 on success, a positive integer when the output is too small to fit the
///   decoded codepoints, and a negative integer on error.
int passgen_utf8_decode(
    uint32_t *output,
    size_t output_len,
    size_t *output_pos,
    uint8_t *output_widths,
    const uint8_t *input,
    size_t input_len,
    size_t *input_pos);

/// Encodes unicode characters into a UTF-8 character sequence.
int passgen_utf8_encode(
    uint8_t *out,
    size_t out_len,
    size_t *out_pos,
    const uint32_t *in,
    size_t in_len,
    size_t *in_pos);
