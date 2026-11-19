/// @file special.h
/// @author Patrick M. Elsen
/// @brief Pattern special data structure (such as `\w{english}` or `\p{uuid}`)
#pragma once
#include <passgen/error.h>
#include <stdint.h>

/// Kind of special pattern
enum passgen_pattern_special_kind {
    /// Wordlist special pattern
    PASSGEN_PATTERN_SPECIAL_WORDLIST,
    /// Markov chain special pattern
    PASSGEN_PATTERN_SPECIAL_MARKOV,
    /// Preset special pattern
    PASSGEN_PATTERN_SPECIAL_PRESET,
};

/// Pattern special, such as `\w{english}`
///
/// Represents a special pattern, such as `\w{name}` (to generate a random
/// word from a word list), `\m{name}` (to generate a random word from a
/// markov chain) or `\p{name}` (to reference a preset pattern).
typedef struct {
    /// Kind of special pattern this is.
    enum passgen_pattern_special_kind kind;
    /// Current length of the parameters string.
    int32_t parameters_len;
    /// Capacity of the parameters string.
    int32_t parameters_cap;
    /// Parameters string, UTF-8 encoded.
    char *parameters;
    /// Where in the pattern this is, from the backslash to the closing
    /// brace, for errors about it.
    passgen_error_offset start;
    /// Where in the pattern this ends, following the closing brace.
    passgen_error_offset end;
    /// What this uses, once the pattern is resolved (see @ref
    /// passgen_resolve). It belongs to the environment.
    union {
        /// The wordlist, for `\w{name}` and `\m{name}`.
        struct passgen_wordlist *wordlist;
        /// The preset, for `\p{name}`.
        struct passgen_preset *preset;
    };
} passgen_pattern_special;

/// Initialise a pattern special with the given char.
///
/// Char can be 'm' for Markov chain, 'w' for word list or 'p' for preset.
///
/// Possible types:
///
/// | Character | Kind | Description |
/// | --- | --- | --- |
/// | `p` | #PASSGEN_PATTERN_SPECIAL_PRESET | Generate the named preset |
/// | `w` | #PASSGEN_PATTERN_SPECIAL_WORDLIST | Generate a random word from the named wordlist |
/// | `m` | #PASSGEN_PATTERN_SPECIAL_MARKOV | Generate a random word using the markov chain parsed from the named wordlist |
///
/// @memberof passgen_pattern_special
/// @param special Special pattern to initialize
/// @param type Type of special pattern to initialize
void passgen_pattern_special_init(
    passgen_pattern_special *special,
    uint32_t type);

/// Free a passgen_pattern_special instance.
///
/// @memberof passgen_pattern_special
/// @param special Special pattern to free
void passgen_pattern_special_free(passgen_pattern_special *special);

/// Add a parameter codepoint to a special pattern.
///
/// @memberof passgen_pattern_special
/// @param special Special pattern to add codepoint to
/// @param codepoint Codepoint to add to the pattern
/// @returns @ref PASSGEN_OK, or @ref PASSGEN_ERROR_OUT_OF_MEMORY
passgen_status passgen_pattern_special_push(
    passgen_pattern_special *special,
    int32_t codepoint);
