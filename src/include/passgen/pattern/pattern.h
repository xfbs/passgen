/// @file pattern.h
/// @author Patrick M. Elsen <pelsen@xfbs.net>
/// @brief Pattern structures and methods
///
/// This is what is inside of a @ref passgen_pattern, which is opaque in the
/// public interface (see `passgen/passgen.h`).
#pragma once

#include "passgen/passgen.h"
#include "passgen/pattern/group.h"

/// Pattern, such as `abc(def|ghi)[a-z]{5}`.
///
/// The root element of a pattern is a group, but it does not need to be
/// enclosed in parantheses.
struct passgen_pattern {
    /// Items in this pattern.
    passgen_pattern_group group;
};

/// Initialize this pattern.
///
/// @memberof passgen_pattern
/// @param pattern Pattern to initialize
void passgen_pattern_init(passgen_pattern *pattern);

/// Release what this pattern holds, but not the pattern itself. This is for
/// patterns which were initialized with @ref passgen_pattern_init or parsed
/// with @ref passgen_pattern_parse, rather than allocated by @ref
/// passgen_parse.
///
/// @memberof passgen_pattern
/// @param pattern Pattern to release
void passgen_pattern_deinit(passgen_pattern *pattern);

/// Parse a pattern into existing storage.
///
/// This is what @ref passgen_parse uses, after allocating a pattern. The
/// pattern has to be released with @ref passgen_pattern_deinit, whether
/// parsing it succeeds or not.
///
/// @memberof passgen_pattern
/// @param pattern Pattern to parse into. Does not need to be initialized.
/// @param error Set if parsing fails, see @ref passgen_parse
/// @param string The pattern to parse, as a NUL-terminated, UTF-8 encoded
///   string
/// @returns Zero on success, nonzero otherwise.
int passgen_pattern_parse(
    passgen_pattern *pattern,
    passgen_error *error,
    const char *string);
