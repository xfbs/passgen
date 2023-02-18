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
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "passgen/pattern/env.h"
#include "passgen/pattern/group.h"
#include "passgen/pattern/segment.h"
#include "passgen/pattern/repeat.h"
#include "passgen/pattern/set.h"
#include "passgen/pattern/special.h"
#include "passgen/pattern/pattern.h"
#include "passgen/pattern/literal.h"

/// Callback for generation. Takes an arbitrary data pointer and the codepoint
/// that was generated.
typedef int passgen_generate_cb(void *data, uint32_t codepoint);

/// Generate a password from a pattern, filling the unicode codepoints into the
/// provided buffer. Will not NULL-terminate anything, returns number of
/// codepoints written.
size_t passgen_generate_fill_unicode(
    const passgen_pattern *pattern,
    passgen_env *env,
    double *entropy,
    uint32_t *buffer,
    size_t len);

/// Generate a password from a patter, filling the result UTF-8 encoded into
/// the provided buffer. Will not NULL-terminate the buffer, returns the number
/// of bytes written.
size_t passgen_generate_fill_utf8(
    const passgen_pattern *pattern,
    passgen_env *env,
    double *entropy,
    uint8_t *buffer,
    size_t len);

/// Generate a password from a patter, filling the result UTF-8 encoded and
/// JSON-escaped into the provided buffer. Will not NULL-terminate the buffer,
/// returns the number of bytes written.
size_t passgen_generate_fill_json_utf8(
    const passgen_pattern *pattern,
    passgen_env *env,
    double *entropy,
    uint8_t *buffer,
    size_t len);

/// Generate a password from a pattern, calling the provided callback function
/// with the provided data pointer as well as each codepoint as it is generated.
int passgen_generate(
    const passgen_pattern *pattern,
    passgen_env *env,
    double *entropy,
    void *data,
    passgen_generate_cb *func);

