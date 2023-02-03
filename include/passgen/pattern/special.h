/// @file special.h
/// @author Patrick M. Elsen
/// @brief Pattern special data structure (such as `\w{english}` or `\p{uuid}`)
#pragma once
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
} passgen_pattern_special;

/// Initialise a pattern special with the given char.
///
/// Char can be 'p' for pronounceable, 'w' for word list or 'p' for preset.
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
void passgen_pattern_special_push(
    passgen_pattern_special *special,
    int32_t codepoint);
