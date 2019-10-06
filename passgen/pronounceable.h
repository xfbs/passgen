#pragma once
#include "passgen/random.h"
#include <stddef.h>

/* list of pronounceable word types that can be generated. */
enum passgen_pronounceable_type {
    PASSGEN_PRONOUNCEABLE_ENGLISH,
    PASSGEN_PRONOUNCEABLE_LAST,
};

/* generate a pronounceable word into buf. writes a maximum of len bytes.
 * returns the number of bytes written. will null-terminate the string, unless
 * it is too small (when return value == len).
 */
size_t passgen_pronounceable(
        enum passgen_pronounceable_type type,
        random_t *rand,
        char *buf,
        size_t len);

/* generates a pronounceable word using the given type that has *exactly*
 * between min and max codepoints. will not write more than *len* characters
 * into the buffer, and exits non-zero if it would. */
int passgen_pronounceable_len(
        enum passgen_pronounceable_type type,
        random_t *rand,
        char *buf,
        size_t len,
        size_t min,
        size_t max,
        size_t tries);
