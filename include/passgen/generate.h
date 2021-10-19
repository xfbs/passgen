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

#include "passgen/data/env.h"

struct passgen_pattern;
struct passgen_chars;
struct passgen_pattern_group;
struct passgen_pattern_item;
struct passgen_pattern_repeat;
struct passgen_pattern_segment;
struct passgen_pattern_set;
struct passgen_pattern_special;

typedef struct passgen_chars passgen_chars_t;

/// Callback for generation. Takes an arbitrary data pointer and the codepoint
/// that was generated.
typedef int passgen_generate_cb(void *data, int32_t codepoint);

/// Generate a password from a pattern, filling the unicode codepoints into the
/// provided buffer. Will not NULL-terminate anything, returns number of
/// codepoints written.
size_t passgen_generate_fill_unicode(
    struct passgen_pattern *pattern,
    passgen_random *rand,
    struct passgen_env *env,
    int32_t *buffer,
    size_t len);

/// Generate a password from a patter, filling the result UTF-8 encoded into
/// the provided buffer. Will not NULL-terminate the buffer, returns the number
/// of bytes written.
size_t passgen_generate_fill_utf8(
    struct passgen_pattern *pattern,
    passgen_random *rand,
    struct passgen_env *env,
    char *buffer,
    size_t len);

/// Generate a password from a pattern, calling the provided callback function
/// with the provided data pointer as well as each codepoint as it is generated.
int passgen_generate(
    struct passgen_pattern *pattern,
    passgen_random *rand,
    struct passgen_env *env,
    void *data,
    passgen_generate_cb *func);

size_t passgen_generate_repeat(
    passgen_random *rand,
    struct passgen_env *env,
    struct passgen_pattern_repeat *repeat);

int passgen_generate_set(
    struct passgen_pattern_set *set,
    passgen_random *rand,
    struct passgen_env *env,
    void *data,
    passgen_generate_cb *func);

int passgen_generate_character(
    passgen_chars_t *character,
    passgen_random *rand,
    struct passgen_env *env,
    void *data,
    passgen_generate_cb *func);

int passgen_generate_special_pronounceable(
    struct passgen_pattern_special *special,
    passgen_random *rand,
    struct passgen_env *env,
    void *data,
    passgen_generate_cb *func);

int passgen_generate_special_wordlist(
    struct passgen_pattern_special *special,
    passgen_random *rand,
    struct passgen_env *env,
    void *data,
    passgen_generate_cb *func);

int passgen_generate_special(
    struct passgen_pattern_special *special,
    passgen_random *rand,
    struct passgen_env *env,
    void *data,
    passgen_generate_cb *func);

int passgen_generate_group(
    struct passgen_pattern_group *group,
    passgen_random *rand,
    struct passgen_env *env,
    void *data,
    passgen_generate_cb *func);

int passgen_generate_item(
    struct passgen_pattern_item *item,
    passgen_random *rand,
    struct passgen_env *env,
    void *data,
    passgen_generate_cb *func);

int passgen_generate_segment(
    struct passgen_pattern_segment *segment,
    passgen_random *rand,
    struct passgen_env *env,
    void *data,
    passgen_generate_cb *func);
