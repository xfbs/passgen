/// @file error.h
/// @brief Creating errors, for use inside of the library.
///
/// All of these accept NULL as the error to write to, as callers of the
/// library may not want one. They return the kind of the error, so that they
/// can be used in a return statement.
#pragma once
#include <passgen/error.h>

/// Set an error to the given kind, without a position, a detail or a cause.
///
/// @param error Error to set, may be NULL
/// @param kind What went wrong
/// @returns The kind
passgen_status passgen_error_set(passgen_error *error, passgen_status kind);

/// Set an error to the given kind, with a position in the pattern.
///
/// @param error Error to set, may be NULL
/// @param kind What went wrong
/// @param start Where the error starts
/// @param end Where the error ends, which is the position following its last
///   codepoint
/// @returns The kind
passgen_status passgen_error_set_at(
    passgen_error *error,
    passgen_status kind,
    passgen_error_offset start,
    passgen_error_offset end);

/// Set what an error is about. The string is copied, and cut off if it does
/// not fit.
///
/// @param error Error to change, may be NULL
/// @param detail NUL-terminated string, may be NULL for none
void passgen_error_detail_set(passgen_error *error, const char *detail);

/// Set an error to the given kind, with another error as what caused it.
///
/// The cause is moved: it is copied into a newly allocated error, along with
/// its own causes, and initialized again. If that cannot be allocated, the
/// cause is released, and the error does not have one.
///
/// @param error Error to set, may be NULL (the cause is released then)
/// @param kind What went wrong
/// @param cause What caused it, may be NULL for nothing
/// @returns The kind
passgen_status passgen_error_wrap(
    passgen_error *error,
    passgen_status kind,
    passgen_error *cause);
