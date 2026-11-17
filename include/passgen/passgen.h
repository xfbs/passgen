/// @file passgen.h
/// @author Patrick M. Elsen <pelsen@xfbs.net>
/// @brief Passgen main interface
#pragma once
#include <stddef.h>
#include <passgen/error.h>
#include <passgen/pattern/pattern.h>

/// Parse a pattern.
///
/// The pattern has to be freed with @ref passgen_pattern_free when it is no
/// longer needed. This is the case even if parsing it fails.
///
/// @param pattern Pointer to a pattern, which is the output of this function.
///   It does not need to be initialized.
/// @param error Pointer to a @ref passgen_error struct. If parsing fails, this
///   has a message describing what is wrong, and where in the string. It is
///   only set in that case, and does not need to be freed.
/// @param string The pattern to parse, as a NUL-terminated, UTF-8 encoded
///   string
/// @returns Zero on success, nonzero otherwise. If it returns zero, it means that @ref pattern
///   contains a fully parsed passgen pattern. If it returns nonzero, it means that @ref error
///   contains information about what went wrong.
int passgen_parse(
    passgen_pattern *pattern,
    passgen_error *error,
    const char *string);
