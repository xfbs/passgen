/// @file generate.h
/// @author Patrick M. Elsen
/// @brief Functions for generating passwords using a parsed pattern.
///
/// This file contains functions used to generate passwords using a parsed
/// `passgen_pattern`. These functions all need some sort of source of
/// randomness, which is a @ref passgen_random object. These can use
/// of the system-specific randomness source, or read from a file, or use a
/// custom randomness generator. 
#pragma once
#include <passgen/export.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "passgen/passgen.h"
#include "passgen/env.h"

/// Callback for generation. Takes an arbitrary data pointer and the codepoint
/// that was generated. If it returns anything other than zero, generating
/// stops, and fails.
typedef int passgen_generate_cb(void *data, uint32_t codepoint);

/// Generate a password from a pattern, filling the unicode codepoints into the
/// provided buffer. Will not NULL-terminate anything.
///
/// See @ref passgen_generate for the parameters.
///
/// @param buffer Where to write the codepoints
/// @param len How many codepoints fit into the buffer
/// @returns The number of codepoints written, or zero if generating failed
///   (which includes the output not fitting into the buffer). Note that zero
///   is also what is returned for a pattern which generates nothing.
PASSGEN_EXPORT size_t passgen_generate_fill_unicode(
    const passgen_pattern *pattern,
    passgen_env *env,
    double *entropy,
    uint32_t *buffer,
    size_t len);

/// Generate a password from a pattern, filling the result UTF-8 encoded into
/// the provided buffer. Will not NULL-terminate the buffer.
///
/// See @ref passgen_generate for the parameters.
///
/// @param buffer Where to write the output
/// @param len How many bytes fit into the buffer
/// @returns The number of bytes written, or zero if generating failed (which
///   includes the output not fitting into the buffer). Note that zero is also
///   what is returned for a pattern which generates nothing.
PASSGEN_EXPORT size_t passgen_generate_fill_utf8(
    const passgen_pattern *pattern,
    passgen_env *env,
    double *entropy,
    uint8_t *buffer,
    size_t len);

/// Generate a password from a pattern, filling the result UTF-8 encoded and
/// escaped for use in a JSON string into the provided buffer. Will not
/// NULL-terminate the buffer.
///
/// See @ref passgen_generate for the parameters.
///
/// @param buffer Where to write the output
/// @param len How many bytes fit into the buffer
/// @returns The number of bytes written, or zero if generating failed (which
///   includes the output not fitting into the buffer). Note that zero is also
///   what is returned for a pattern which generates nothing.
PASSGEN_EXPORT size_t passgen_generate_fill_json_utf8(
    const passgen_pattern *pattern,
    passgen_env *env,
    double *entropy,
    uint8_t *buffer,
    size_t len);

/// Generate a password from a pattern, calling the provided callback function
/// with the provided data pointer as well as each codepoint as it is generated.
///
/// @param pattern A pattern which was parsed successfully
/// @param env The environment, which has the source of randomness and the
///   wordlists
/// @param entropy If not NULL, this is set to the entropy of what was
///   generated, in bits. This is the logarithm to base two of the number of
///   equally likely choices that were made to generate it: with 64 bits,
///   someone who knows the pattern needs up to 2^64 guesses to find it. For a
///   pattern which can generate the same output in more than one way (such as
///   `[aa]`), this is more than the entropy of the output.
/// @param data Passed to the callback as it is
/// @param func Called for every codepoint which is generated
/// @returns Zero on success, nonzero on failure. Generating fails if the
///   pattern uses a wordlist which is not in the environment or which is empty,
///   if groups are nested more deeply than the depth limit of the environment,
///   or if the callback returns nonzero.
PASSGEN_EXPORT int passgen_generate(
    const passgen_pattern *pattern,
    passgen_env *env,
    double *entropy,
    void *data,
    passgen_generate_cb *func);
