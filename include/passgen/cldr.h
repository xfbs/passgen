#pragma once
#include <stdint.h>

#define PASSGEN_CLDR_LANGUAGE_CODE_CHARS 12
#define PASSGEN_CLDR_DATA_DIR "/usr/share/unicode/cldr"

/// Return values for CLDR routines.
enum passgen_cldr_ret {
    /// Success
    PASSGEN_CLDR_OKAY,
    /// Error allocating memory.
    PASSGEN_CLDR_MEMORY,
    /// Destination size too small.
    PASSGEN_CLDR_SIZE,
    /// CLDR Data not found.
    PASSGEN_CLDR_DATA,
    /// The requested data was not found.
    PASSGEN_CLDR_NOTFOUND,
};

/// Look up the country code for a language name. For example, "German" has the
/// country code "de". Needs to specify a language (for example english) in which
/// the country name is expressed. Matches case-independent.
///
/// If `language` is NULL, it uses the current language as set by `LANGUAGE`,
/// or if that does not exist, it falls back to using `en`.
///
/// If `codes` is NULL, it will allocate an array of appropriate size.
/// If `count` is 0, it will keep going until it gets to a NULL pointer,
/// otherwise it only looks up a maximum of `count` country names.
enum passgen_cldr_ret passgen_cldr_language_code(
        char ***codes,
        const char *language,
        const uint32_t **names,
        size_t count);

/// Get the exemplarCharacters for a given language (along with other features).
///
/// When count is zero, a new array is allocated to hold the codepoints. If it
/// is nonzero, the existing array is filled. If it is not large enough, the
/// return value is `PASSGEN_CLDR_SIZE`.
///
/// Code is a country code such as `de` or `zh`. `options` can be NULL or a
/// string containing textual options.
enum passgen_cldr_ret passgen_cldr_exemplar(
        size_t *count,
        int32_t **codepoints,
        const char *code,
        const char *options);
