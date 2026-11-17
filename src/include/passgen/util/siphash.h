/// @file siphash.h
/// @brief SipHash reference C implementation
/// @author Copyright (c) 2012-2021 Jean-Philippe Aumasson <jeanphilippe.aumasson@gmail.com>
/// @author Copyright (c) 2012-2014 Daniel J. Bernstein <djb@cr.yp.to>
/// @license CC0
///
/// To the extent possible under law, the author(s) have dedicated all copyright
/// and related and neighboring rights to this software to the public domain
/// worldwide. This software is distributed without any warranty.
/// You should have received a copy of the CC0 Public Domain Dedication along
/// with
/// this software. If not, see
/// <http://creativecommons.org/publicdomain/zero/1.0/>.

#pragma once
#include <inttypes.h>
#include <string.h>

/// SipHash function
/// High-speed secure pseudorandom function for short messages.
/// @param in Pointer to data to hash
/// @param inlen Length of input data
/// @param k Pointer to key, must be 16 bytes
/// @param out Pointer to output
/// @param outlen Length (in bytes) of output, must be 8 or 16.
/// @returns 0 on success
int passgen_siphash(
    const void *in,
    const size_t inlen,
    const void *k,
    uint8_t *out,
    const size_t outlen);
