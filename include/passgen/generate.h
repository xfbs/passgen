#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "passgen/data/pattern.h"
#include "passgen/pronounceable.h"
#include "passgen/random.h"

struct passgen_pattern;
struct passgen_pattern_char;
struct passgen_pattern_group;
struct passgen_pattern_item;
struct passgen_pattern_repeat;
struct passgen_pattern_segment;
struct passgen_pattern_set;
struct passgen_pattern_special;

struct passgen_pattern_repeat;
struct passgen_pattern_set;
struct passgen_pattern_char;
struct passgen_pattern_group;
struct passgen_pattern_item;
struct passgen_pattern_segment;
struct passgen_pattern_special;

struct pattern_env {
  bool find_complexity;
  double complexity;
  size_t pronounceable_limit;
  enum passgen_pronounceable_type pronounceable_type;
};

/// Callback for generation. Takes an arbitrary data pointer and the codepoint
/// that was generated.
typedef int passgen_generate_cb(void *data, int32_t codepoint);

/// Generate a password from a pattern, filling the unicode codepoints into the
/// provided buffer. Will not NULL-terminate anything, returns number of
/// codepoints written.
size_t passgen_generate_fill_unicode(
    struct passgen_pattern *pattern,
    passgen_random_t *rand,
    struct pattern_env *env,
    int32_t *buffer,
    size_t len);

/// Generate a password from a patter, filling the result UTF-8 encoded into
/// the provided buffer. Will not NULL-terminate the buffer, returns the number
/// of bytes written.
size_t passgen_generate_fill_utf8(
    struct passgen_pattern *pattern,
    passgen_random_t *rand,
    struct pattern_env *env,
    char *buffer,
    size_t len);

/// Generate a password from a pattern, calling the provided callback function
/// with the provided data pointer as well as each codepoint as it is generated.
int passgen_generate(
    struct passgen_pattern *pattern,
    passgen_random_t *rand,
    struct pattern_env *env,
    void *data,
    passgen_generate_cb *func);

size_t passgen_generate_repeat(
    passgen_random_t *rand,
    struct pattern_env *env,
    struct passgen_pattern_repeat *repeat);

int passgen_generate_set(
    struct passgen_pattern_set *set,
    passgen_random_t *rand,
    struct pattern_env *env,
    void *data,
    passgen_generate_cb *func);

int passgen_generate_character(
    struct passgen_pattern_char *character,
    passgen_random_t *rand,
    struct pattern_env *env,
    void *data,
    passgen_generate_cb *func);

int passgen_generate_special_pronounceable(
    struct passgen_pattern_special *special,
    passgen_random_t *rand,
    struct pattern_env *env,
    void *data,
    passgen_generate_cb *func);

int passgen_generate_special_wordlist(
    struct passgen_pattern_special *special,
    passgen_random_t *rand,
    struct pattern_env *env,
    void *data,
    passgen_generate_cb *func);

int passgen_generate_special(
    struct passgen_pattern_special *special,
    passgen_random_t *rand,
    struct pattern_env *env,
    void *data,
    passgen_generate_cb *func);

int passgen_generate_group(
    struct passgen_pattern_group *group,
    passgen_random_t *rand,
    struct pattern_env *env,
    void *data,
    passgen_generate_cb *func);

int passgen_generate_item(
    struct passgen_pattern_item *item,
    passgen_random_t *rand,
    struct pattern_env *env,
    void *data,
    passgen_generate_cb *func);

int passgen_generate_segment(
    struct passgen_pattern_segment *segment,
    passgen_random_t *rand,
    struct pattern_env *env,
    void *data,
    passgen_generate_cb *func);
