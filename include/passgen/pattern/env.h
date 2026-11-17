/// @file env.h
/// @author Patrick M. Elsen <pelsen@xfbs.net>
/// @brief Passgen environment

#pragma once

#include "passgen/container/hashmap.h"
#include "passgen/random.h"
#include "passgen/pattern/pattern.h"
#include "passgen/pattern/preset.h"
#include <stdio.h>

/// Environment that is needed when generating patterns.
///
/// This allows for passing data to the generation functions, such as the available wordlists
/// for word-based patterns and the randomness generator.
typedef struct passgen_env {
    /// Recursion depth limit for generation
    size_t depth_limit;
    /// Wordlists that are loaded
    passgen_hashmap wordlists;
    /// Presets that are loaded
    passgen_hashmap presets;
    /// Source of randomness
    passgen_random *random;
} passgen_env;

/// Initialize environment.
///
/// The environment takes ownership of the source of randomness: @ref
/// passgen_env_free closes it, and frees it. So it has to be one which was
/// allocated, by passing NULL to the function which opened it. To keep
/// ownership of it, set the `random` member to NULL before freeing the
/// environment.
///
/// @memberof passgen_env
/// @param env Environment to initialize
/// @param random Source of randomness to generate from, may be NULL if it is
///   set later
void passgen_env_init(passgen_env *env, passgen_random *random);

/// Adds a wordlist, which patterns can then refer to by its name. If there is
/// a wordlist with this name already, it is replaced.
///
/// The wordlist is not read right away, but when it is first used. The
/// environment takes ownership of both the name and the file.
///
/// @memberof passgen_env
/// @param env Environment to add the wordlist to
/// @param name Name of the wordlist, as a NUL-terminated string which was
///   allocated with malloc(). It is freed along with the environment.
/// @param file File to read the wordlist from, with one word per line. It has
///   to be seekable. It is closed once it was read, or along with the
///   environment.
/// @param markov_length Length of the Markov chain to build from the
///   wordlist when it is used with one, between 1 and 255
/// @returns Zero on success, nonzero on failure
int passgen_env_wordlist_add(passgen_env *env, char *name, FILE *file, size_t markov_length);

/// Loads the given configuration. This is not implemented, and always fails.
///
/// @memberof passgen_env
int passgen_env_config(passgen_env env, FILE *file);

/// Free environment, along with its wordlists and its source of randomness
/// (see @ref passgen_env_init).
///
/// @memberof passgen_env
void passgen_env_free(passgen_env *env);
