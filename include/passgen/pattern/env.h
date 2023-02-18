/// @file env.h
/// @author Patrick M. Elsen <pelsen@xfbs.net>
/// @brief Passgen environment

#pragma once

#include "passgen/container/hashmap.h"
#include "passgen/util/random.h"
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
/// @memberof passgen_env
void passgen_env_init(passgen_env *env, passgen_random *random);

/// Loads the given wordlist.
///
/// @memberof passgen_env
int passgen_env_wordlist_add(passgen_env *env, char *name, FILE *file, size_t markov_length);

/// Loads the given configuration.
///
/// @memberof passgen_env
int passgen_env_config(passgen_env env, FILE *file);

/// Free environment.
///
/// @memberof passgen_env
void passgen_env_free(passgen_env *env);
