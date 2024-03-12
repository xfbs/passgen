/// @file passgen.h
/// @author Patrick M. Elsen <pelsen@xfbs.net>
/// @brief Passgen main interface
#pragma once
#include <stddef.h>
#include <passgen/error.h>
#include <passgen/pattern/pattern.h>

/// Parse a pattern.
///
/// @param pattern Pointer to a valid pattern. Does not need to be initialized. This is
///   the output of the function.
/// @param error Pointer to a @ref passgen_error struct. In case of an error, this is where
///   information about the location and a descriptive message will be returned.
/// @param string A valid passgen pattern string
/// @returns Zero on success, nonzero otherwise. If it returns zero, it means that @ref pattern
///   contains a fully parsed passgen pattern. If it returns nonzero, it means that @ref error
///   contains information about what went wrong.
int passgen_parse(
    passgen_pattern *pattern,
    passgen_error *error,
    const char *string);
