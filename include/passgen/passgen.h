/// @file passgen.h
/// @author Patrick M. Elsen <pelsen@xfbs.net>
/// @brief Parsing patterns, and all of the rest of the interface.
///
/// Including this header is enough to use all of libpassgen: it includes the
/// other public headers.
#pragma once
#include <passgen/error.h>
#include <passgen/export.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/// @struct passgen_pattern
/// A parsed pattern, such as `abc(def|ghi)[a-z]{5}`.
///
/// Patterns are created by parsing them with @ref passgen_parse, and have to
/// be freed with @ref passgen_pattern_free. What is inside of them is not part
/// of the public interface.
typedef struct passgen_pattern passgen_pattern;

/// Parse a pattern.
///
/// @param pattern Where to store the pattern. If parsing succeeds, this is set
///   to a newly allocated pattern, which has to be freed with @ref
///   passgen_pattern_free when it is no longer needed. If parsing fails, it is
///   set to NULL, and there is nothing to free.
/// @param error If parsing fails, this is set to what is wrong, and where in
///   the string. It then has to be released with @ref passgen_error_free. May
///   be NULL.
/// @param string The pattern to parse, as a NUL-terminated, UTF-8 encoded
///   string
/// @returns @ref PASSGEN_OK on success, and what is wrong with the pattern
///   otherwise.
PASSGEN_EXPORT passgen_status passgen_parse(
    passgen_pattern **pattern,
    passgen_error *error,
    const char *string);

/// Free a pattern which was created by @ref passgen_parse.
///
/// @relatesalso passgen_pattern
/// @param pattern Pattern to free. May be NULL, in which case this does
///   nothing.
PASSGEN_EXPORT void passgen_pattern_free(passgen_pattern *pattern);

#ifdef __cplusplus
}
#endif

// the rest of the interface. this comes last, as generate.h needs what is
// declared above.
#include <passgen/allocator.h>
#include <passgen/config.h>
#include <passgen/env.h>
#include <passgen/generate.h>
#include <passgen/random.h>
#include <passgen/version.h>
