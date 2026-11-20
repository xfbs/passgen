/// @file error.h
/// @author Patrick M. Elsen <pelsen@xfbs.net>
/// @brief Errors, and what they are caused by.
///
/// Functions which can fail return a @ref passgen_status, which says what went
/// wrong. Those which have more to say take a pointer to a @ref passgen_error
/// as well, which they fill in when they fail:
///
/// ```c
/// passgen_pattern *pattern;
/// passgen_error error;
///
/// if(passgen_parse(&pattern, &error, "[a-z") != PASSGEN_OK) {
///     passgen_error_print(&error, stderr);
///     passgen_error_free(&error);
/// }
/// ```
///
/// The pointer to the error may be NULL, if the status is all that is needed.
#pragma once
#include <passgen/export.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/// What went wrong, or @ref PASSGEN_OK if nothing did.
///
/// The values may change between versions, so use the names. Use @ref
/// passgen_status_string to get a description.
typedef enum passgen_status {
    /// Nothing went wrong.
    PASSGEN_OK = 0,
    /// Memory could not be allocated.
    PASSGEN_ERROR_OUT_OF_MEMORY,
    /// Something which should not be possible happened. This is a bug in the
    /// library.
    PASSGEN_ERROR_INTERNAL,
    /// A function was given an argument which is not valid, such as a length
    /// of a Markov chain of zero. This is a mistake in the program which uses
    /// the library.
    PASSGEN_ERROR_INVALID_ARGUMENT,
    /// A file could not be read. The detail of the error is the reason.
    PASSGEN_ERROR_IO,

    /// The pattern is not valid UTF-8, or a word of a wordlist is not (then
    /// the detail of the error is its line).
    PASSGEN_ERROR_UTF8,
    /// A backslash is followed by something which is not an escape sequence,
    /// such as `\q`.
    PASSGEN_ERROR_ESCAPE,
    /// The pattern ends in the middle of an escape sequence, such as `abc\`.
    PASSGEN_ERROR_ESCAPE_UNFINISHED,
    /// A unicode escape is malformed, such as `\u{xyz}`.
    PASSGEN_ERROR_UNICODE_ESCAPE,
    /// A unicode escape is not a valid codepoint, such as `\u{110000}`.
    PASSGEN_ERROR_UNICODE_RANGE,
    /// A group is closed which was never opened, such as `abc)`.
    PASSGEN_ERROR_GROUP_UNOPENED,
    /// A group is not closed, such as `(abc`.
    PASSGEN_ERROR_GROUP_UNCLOSED,
    /// A set is not closed, such as `[abc`.
    PASSGEN_ERROR_SET_UNCLOSED,
    /// A set does not have any characters, such as `[]`.
    PASSGEN_ERROR_SET_EMPTY,
    /// A range in a set does not have an end, or ends before it starts, such
    /// as `[a-]` or `[z-a]`.
    PASSGEN_ERROR_SET_RANGE,
    /// A repetition or a multiplier is not valid: it is not a number, the
    /// number is too large, the maximum is smaller than the minimum (such as
    /// `a{5,2}`), or it is not closed.
    PASSGEN_ERROR_REPEAT,
    /// A segment of a group has more than one weight, such as `({2}{3}a|b)`.
    PASSGEN_ERROR_WEIGHT,
    /// A question mark does not follow anything which it could make
    /// optional, such as `?a`.
    PASSGEN_ERROR_OPTIONAL,
    /// A wordlist is referred to without a name in braces following it, such
    /// as `\wabc`, `\w{abc` or `\w{}`.
    PASSGEN_ERROR_SPECIAL,
    /// The length of a Markov chain is not a number from 1 to 8, such as in
    /// `\m{english:0}` or `\m{english:abc}`.
    PASSGEN_ERROR_MARKOV_LENGTH,
    /// Something which is not a Markov chain has a parameter after a colon,
    /// such as `\w{english:2}`. To have a colon in a name, escape it.
    PASSGEN_ERROR_SPECIAL_PARAMETER,
    /// Groups are nested more than 1024 deep, such as in a pattern which
    /// starts with 1025 opening parentheses.
    PASSGEN_ERROR_DEPTH_LIMIT,
    /// Generating takes more steps than the limit, such as `(a{1000}){1000}`
    /// with the default limit. See @ref passgen_env_step_limit_set.
    PASSGEN_ERROR_STEP_LIMIT,

    /// The pattern uses a wordlist which is not in the environment. The
    /// detail of the error is its name.
    PASSGEN_ERROR_WORDLIST_NOT_FOUND,
    /// The pattern uses a wordlist which does not have any words. The detail
    /// of the error is its name.
    PASSGEN_ERROR_WORDLIST_EMPTY,
    /// The pattern uses a wordlist which could not be read, or which has
    /// words that are not valid UTF-8. The detail of the error is its name,
    /// and its cause is why.
    PASSGEN_ERROR_WORDLIST_READ,
    /// The pattern uses a preset, such as `\p{name}`, which is not in the
    /// environment. The detail of the error is its name.
    PASSGEN_ERROR_PRESET_NOT_FOUND,
    /// The pattern uses a preset which cannot be used, because its pattern
    /// is not valid, or something which it uses cannot be. The detail of the
    /// error is its name, and its cause is why. The position of the cause is
    /// in the pattern of the preset.
    PASSGEN_ERROR_PRESET_INVALID,
    /// The pattern uses a preset which uses itself, also through other
    /// presets, so that it would never end. The detail of the error is its
    /// name.
    PASSGEN_ERROR_PRESET_CYCLE,
    /// All of the segments of a group have a multiplier of zero, such as
    /// `({0}a)`, so there is nothing to choose from. This is an error when
    /// parsing, also for a group which is repeated zero times.
    PASSGEN_ERROR_GROUP_EMPTY,
    /// The source of randomness did not have as much data as was needed.
    PASSGEN_ERROR_RANDOM,
    /// What was generated does not fit into the buffer for it.
    PASSGEN_ERROR_BUFFER_TOO_SMALL,
    /// The callback which is given what is generated asked to stop.
    PASSGEN_ERROR_ABORTED,
    /// A file does not have the hash it is expected to have, such as a
    /// wordlist (see @ref passgen_env_wordlist_add_path).
    PASSGEN_ERROR_HASH,
} passgen_status;

/// A position in a pattern. Both count from zero.
typedef struct {
    /// How many codepoints come before it.
    size_t codepoint;
    /// How many bytes come before it, in the UTF-8 encoded pattern.
    size_t byte;
} passgen_error_offset;

/// An error, with what it was caused by.
///
/// The error itself is owned by whoever declares it, typically on the stack.
/// It is only written to when a function fails. After that, it has to be
/// released with @ref passgen_error_free, as it may have causes, which are
/// allocated.
typedef struct passgen_error {
    /// What went wrong.
    passgen_status kind;
    /// Whether @ref start and @ref end are set. They are for errors in a
    /// pattern.
    bool has_position;
    /// Where in the pattern the error starts.
    passgen_error_offset start;
    /// Where in the pattern the error ends. This is the position following
    /// its last codepoint.
    passgen_error_offset end;
    /// What the error is about, such as the name of a wordlist, as a
    /// NUL-terminated string. Empty if there is nothing to add. Names which
    /// do not fit are cut off.
    char detail[48];
    /// What caused this error, or NULL if it is not known or there is nothing
    /// behind it. Owned by this error.
    struct passgen_error *cause;
} passgen_error;

/// Describes a status in a few words, in lowercase and without punctuation,
/// such as `set is empty`.
///
/// @param status The status to describe
/// @returns A string which is valid forever. Never NULL.
PASSGEN_EXPORT const char *passgen_status_string(passgen_status status);

/// Initialize an error to having nothing wrong.
///
/// This is not needed before passing an error to a function. It is useful to
/// be able to call @ref passgen_error_free whether or not there was an error.
///
/// @relatesalso passgen_error
/// @param error Error to initialize
PASSGEN_EXPORT void passgen_error_init(passgen_error *error);

/// Release what an error holds, which are its causes, and initialize it
/// again. The error itself is not freed.
///
/// @relatesalso passgen_error
/// @param error Error to release. May be NULL, in which case this does
///   nothing.
PASSGEN_EXPORT void passgen_error_free(passgen_error *error);

/// Print an error and its causes, one per line, such as:
///
/// ```
/// wordlist could not be read: english
/// caused by: not valid UTF-8
/// ```
///
/// This does not print where in a pattern the error is, see @ref
/// passgen_error::start for that.
///
/// @relatesalso passgen_error
/// @param error Error to print
/// @param file Where to print it to, such as `stderr`
PASSGEN_EXPORT void passgen_error_print(const passgen_error *error, FILE *file);

#ifdef __cplusplus
}
#endif
