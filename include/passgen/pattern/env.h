/// @file env.h
/// @author Patrick M. Elsen <pelsen@xfbs.net>
/// @brief Passgen environment

#pragma once

#include "passgen/util/hashmap.h"
#include "passgen/util/random.h"

/// Environment that is needed when generating patterns.
///
/// This allows for passing data to the generation functions, such as the available wordlists
/// for word-based patterns and the randomness generator.
struct passgen_env {
  bool find_entropy;
  double entropy;
  passgen_hashmap wordlists;
  passgen_random *random;
};

