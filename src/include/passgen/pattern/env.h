/// @file env.h
/// @author Patrick M. Elsen <pelsen@xfbs.net>
/// @brief Passgen environment
///
/// This is what is inside of a @ref passgen_env, which is opaque in the
/// public interface (see `passgen/env.h`).

#pragma once

#include "passgen/container/hashmap.h"
#include "passgen/env.h"
#include "passgen/random.h"

/// Environment that is needed when generating patterns.
struct passgen_env {
    /// Recursion depth limit for generation
    size_t depth_limit;
    /// How many steps generating may take, see passgen_env_step_limit_set()
    size_t step_limit;
    /// Wordlists that are loaded. The keys are the names, which are owned by
    /// the environment, as are the wordlists.
    passgen_hashmap wordlists;
    /// Presets that are loaded
    passgen_hashmap presets;
    /// Source of randomness. This is not owned by the environment.
    passgen_random *random;
};
