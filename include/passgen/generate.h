/// @file generate.h
/// @author Patrick M. Elsen
/// @brief Generating from a pattern.
///
/// Generating takes a pattern (see @ref passgen_parse) and an environment
/// (see @ref passgen_env_new), which has the source of randomness and the
/// wordlists. What is generated can be written into a buffer, as codepoints
/// or as UTF-8, or be passed to a callback one codepoint at a time.
///
/// If generating fails, whatever was written or passed on so far is not
/// what the pattern generates, and has to be thrown away.
#pragma once
#include <passgen/export.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "passgen/passgen.h"
#include "passgen/env.h"

/// Callback which is given what is generated, one codepoint at a time.
///
/// @param data What was passed to @ref passgen_generate as it is
/// @param codepoint The codepoint which was generated
/// @returns Zero to go on. Anything else stops generating, which then fails
///   with @ref PASSGEN_ERROR_ABORTED.
typedef int passgen_generate_cb(void *data, uint32_t codepoint);

/// Generates from a pattern into a buffer of codepoints. This does not
/// terminate it with a zero.
///
/// @param pattern A pattern which was parsed successfully
/// @param env The environment, which has the source of randomness and the
///   wordlists
/// @param error If generating fails, this is set to what went wrong. It then
///   has to be released with @ref passgen_error_free. May be NULL.
/// @param entropy If not NULL, set to the entropy of what was generated, in
///   bits (see @ref passgen_generate)
/// @param buffer Where to write the codepoints
/// @param len How many codepoints fit into the buffer
/// @param written Set to the number of codepoints written, or to zero if
///   generating fails. May be NULL.
/// @returns @ref PASSGEN_OK on success. If what is generated does not fit
///   into the buffer, @ref PASSGEN_ERROR_BUFFER_TOO_SMALL. Otherwise, what
///   @ref passgen_generate returns.
PASSGEN_EXPORT passgen_status passgen_generate_fill_unicode(
    const passgen_pattern *pattern,
    passgen_env *env,
    passgen_error *error,
    double *entropy,
    uint32_t *buffer,
    size_t len,
    size_t *written);

/// Generates from a pattern into a buffer, encoded as UTF-8. This does not
/// terminate it with a NUL byte, so leave room for one if it is needed.
///
/// @param pattern A pattern which was parsed successfully
/// @param env The environment, which has the source of randomness and the
///   wordlists
/// @param error If generating fails, this is set to what went wrong. It then
///   has to be released with @ref passgen_error_free. May be NULL.
/// @param entropy If not NULL, set to the entropy of what was generated, in
///   bits (see @ref passgen_generate)
/// @param buffer Where to write the output
/// @param len How many bytes fit into the buffer
/// @param written Set to the number of bytes written, or to zero if
///   generating fails. May be NULL.
/// @returns @ref PASSGEN_OK on success. If what is generated does not fit
///   into the buffer, @ref PASSGEN_ERROR_BUFFER_TOO_SMALL. Otherwise, what
///   @ref passgen_generate returns.
PASSGEN_EXPORT passgen_status passgen_generate_fill_utf8(
    const passgen_pattern *pattern,
    passgen_env *env,
    passgen_error *error,
    double *entropy,
    uint8_t *buffer,
    size_t len,
    size_t *written);

/// Generates from a pattern into a buffer, encoded as UTF-8 and escaped to be
/// put into a JSON string: quotes, backslashes and control characters are
/// escaped. The quotes around the string are not written. This does not
/// terminate it with a NUL byte, so leave room for one if it is needed.
///
/// @param pattern A pattern which was parsed successfully
/// @param env The environment, which has the source of randomness and the
///   wordlists
/// @param error If generating fails, this is set to what went wrong. It then
///   has to be released with @ref passgen_error_free. May be NULL.
/// @param entropy If not NULL, set to the entropy of what was generated, in
///   bits (see @ref passgen_generate)
/// @param buffer Where to write the output
/// @param len How many bytes fit into the buffer
/// @param written Set to the number of bytes written, or to zero if
///   generating fails. May be NULL.
/// @returns @ref PASSGEN_OK on success. If what is generated does not fit
///   into the buffer, @ref PASSGEN_ERROR_BUFFER_TOO_SMALL. Otherwise, what
///   @ref passgen_generate returns.
PASSGEN_EXPORT passgen_status passgen_generate_fill_json_utf8(
    const passgen_pattern *pattern,
    passgen_env *env,
    passgen_error *error,
    double *entropy,
    uint8_t *buffer,
    size_t len,
    size_t *written);

/// Generates from a pattern, passing each codepoint to a callback as it is
/// generated. This is what the other functions for generating are built on,
/// and the only one for output which does not have a size limit.
///
/// @param pattern A pattern which was parsed successfully
/// @param env The environment, which has the source of randomness and the
///   wordlists
/// @param error If generating fails, this is set to what went wrong. It then
///   has to be released with @ref passgen_error_free. May be NULL.
/// @param entropy If not NULL, this is set to the entropy of what was
///   generated, in bits. This is the logarithm to base two of the number of
///   equally likely choices that were made to generate it: with 64 bits,
///   someone who knows the pattern needs up to 2^64 guesses to find it. For a
///   pattern which can generate the same output in more than one way (such as
///   `[aa]`), this is more than the entropy of the output. It does not mean
///   anything if generating fails.
/// @param data Passed to the callback as it is
/// @param func Called for every codepoint which is generated
/// @returns @ref PASSGEN_OK on success, and otherwise why generating failed:
///   - @ref PASSGEN_ERROR_WORDLIST_NOT_FOUND, @ref PASSGEN_ERROR_WORDLIST_EMPTY
///     or @ref PASSGEN_ERROR_WORDLIST_READ, if there is a problem with a
///     wordlist which the pattern uses,
///   - @ref PASSGEN_ERROR_DEPTH_LIMIT, if groups are nested more deeply than
///     the depth limit of the environment,
///   - @ref PASSGEN_ERROR_GROUP_EMPTY, if there is a group with nothing to
///     choose from,
///   - @ref PASSGEN_ERROR_RANDOM, if the source of randomness ran out (see
///     @ref passgen_random_failed),
///   - @ref PASSGEN_ERROR_ABORTED, if the callback returns nonzero.
PASSGEN_EXPORT passgen_status passgen_generate(
    const passgen_pattern *pattern,
    passgen_env *env,
    passgen_error *error,
    double *entropy,
    void *data,
    passgen_generate_cb *func);
