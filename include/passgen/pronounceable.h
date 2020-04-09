#pragma once
#include "passgen/random.h"
#include <stddef.h>

/* list of pronounceable word types that can be generated. */
enum passgen_pronounceable_type {
  PASSGEN_PRONOUNCEABLE_ENGLISH,
  PASSGEN_PRONOUNCEABLE_LATIN,
  PASSGEN_PRONOUNCEABLE_LAST,
};

typedef struct {
  const char *name;
  enum passgen_pronounceable_type type;
} passgen_pronounceable_map_t;

extern passgen_pronounceable_map_t passgen_pronounceable_map[];

enum passgen_pronounceable_type
passgen_pronounceable_lookup(size_t length, const char *name);

/* generate a pronounceable word into buf. writes a maximum of len bytes.
 * returns the number of bytes written. will null-terminate the string, unless
 * it is too small (when return value == len).
 */
size_t passgen_pronounceable(
    enum passgen_pronounceable_type type,
    random_t *rand,
    int32_t *buf,
    size_t len);

/* generates a pronounceable word using the given type that has *exactly*
 * between min and max codepoints. will not write more than *len* characters
 * into the buffer, and exits non-zero if it would. */
size_t passgen_pronounceable_len(
    enum passgen_pronounceable_type type,
    random_t *rand,
    int32_t *buf,
    size_t min,
    size_t max,
    size_t tries);
