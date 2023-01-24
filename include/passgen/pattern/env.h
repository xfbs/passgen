/// @file env.h
/// @author Patrick M. Elsen <pelsen@xfbs.net>
/// @brief Passgen environment

#pragma once

#include "passgen/util/hashmap.h"
#include "passgen/util/random.h"
#include "passgen/pattern/pattern.h"
#include <stdio.h>

typedef struct passgen_preset {
    const char *name;
    const char *preset;
    bool parsed;
    passgen_pattern pattern;
} passgen_preset;

/// Environment that is needed when generating patterns.
///
/// This allows for passing data to the generation functions, such as the available wordlists
/// for word-based patterns and the randomness generator.
typedef struct passgen_env {
  bool find_entropy;
  double entropy;
  size_t depth;
  passgen_hashmap wordlists;
  passgen_hashmap presets;
  passgen_random *random;
} passgen_env;

/// Initialize environment.
void passgen_env_init(passgen_env *env, passgen_random *random);

/// Loads the given wordlist.
int passgen_env_wordlist_add(passgen_env *env, const char *name, FILE *file, size_t markov_length);

/// Loads the given configuration.
int passgen_env_config(passgen_env env, FILE *file);

/// Free environment.
void passgen_env_free(passgen_env *env);
