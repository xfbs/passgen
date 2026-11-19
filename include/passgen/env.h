/// @file env.h
/// @author Patrick M. Elsen <pelsen@xfbs.net>
/// @brief Environment for generating from patterns.
#pragma once
#include <passgen/error.h>
#include <passgen/export.h>

#include <passgen/random.h>
#include <stddef.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/// @struct passgen_env
/// Environment that is needed when generating from patterns.
///
/// This has what a pattern needs to generate something, apart from the
/// pattern itself: the source of randomness, and the wordlists which patterns
/// can refer to. It also has the limits for generating.
///
/// Environments are created with @ref passgen_env_new, and have to be freed
/// with @ref passgen_env_free. What is inside of them is not part of the
/// public interface.
typedef struct passgen_env passgen_env;

/// Create an environment.
///
/// The environment does not take ownership of the source of randomness. It
/// has to stay open for as long as the environment is used to generate
/// anything, and it is not closed when the environment is freed.
///
/// @relatesalso passgen_env
/// @param random Source of randomness to generate from. May be NULL if it is
///   set later, with @ref passgen_env_random_set.
/// @returns The environment, or NULL if it could not be allocated.
PASSGEN_EXPORT passgen_env *passgen_env_new(passgen_random *random);

/// Free an environment, along with its wordlists. This does not close the
/// source of randomness.
///
/// @relatesalso passgen_env
/// @param env Environment to free. May be NULL, in which case this does
///   nothing.
PASSGEN_EXPORT void passgen_env_free(passgen_env *env);

/// Set the source of randomness to generate from.
///
/// The environment does not take ownership of it, see @ref passgen_env_new.
///
/// @relatesalso passgen_env
/// @param env Environment to change
/// @param random Source of randomness to generate from
PASSGEN_EXPORT void passgen_env_random_set(passgen_env *env, passgen_random *random);

/// Set how deeply the groups in a pattern may be nested when generating.
/// Generating fails for patterns which are nested more deeply. The default is
/// 1024.
///
/// @relatesalso passgen_env
/// @param env Environment to change
/// @param depth_limit How deeply groups may be nested. The pattern itself
///   counts as a group, so with zero, nothing can be generated.
PASSGEN_EXPORT void passgen_env_depth_limit_set(passgen_env *env, size_t depth_limit);

/// Set how many steps generating from a pattern may take. Generating fails
/// with @ref PASSGEN_ERROR_STEP_LIMIT for patterns which take more. The
/// default is 1000000.
///
/// Without a limit, a short pattern can take practically forever:
/// `(){999999999}` repeats an empty group that many times, and repetitions
/// which are nested multiply. This matters when patterns come from someone
/// else.
///
/// Every time something in a pattern is generated is a step: a character of
/// a set, a run of literal characters, a word, or a group, and each of its
/// repetitions counts. So `[a-z]{20}` takes 20 steps, and `(ab){3}` takes 3
/// for the group and 3 for what is inside of it.
///
/// @relatesalso passgen_env
/// @param env Environment to change
/// @param step_limit How many steps generating may take. Use `SIZE_MAX` for
///   practically no limit.
PASSGEN_EXPORT void passgen_env_step_limit_set(passgen_env *env, size_t step_limit);

/// Adds a wordlist, which patterns can then refer to by its name. If there is
/// a wordlist with this name already, it is replaced.
///
/// The wordlist is not read right away, but when it is first used. So the
/// environment takes ownership of the file: it is closed once it was read, or
/// when the environment is freed. This is also the case if adding the wordlist
/// fails.
///
/// @relatesalso passgen_env
/// @param env Environment to add the wordlist to
/// @param name Name of the wordlist, as a NUL-terminated string. It is
///   copied.
/// @param file File to read the wordlist from, with one word per line. It has
///   to be seekable.
/// @param markov_length Length of the Markov chain to build from the
///   wordlist when it is used with one, between 1 and 255
/// @returns @ref PASSGEN_OK, @ref PASSGEN_ERROR_OUT_OF_MEMORY, or
///   @ref PASSGEN_ERROR_INVALID_ARGUMENT if the length of the Markov chain is
///   not between 1 and 255. A file which cannot be read is not noticed here,
///   but when generating, which then fails with
///   @ref PASSGEN_ERROR_WORDLIST_READ.
PASSGEN_EXPORT passgen_status passgen_env_wordlist_add(
    passgen_env *env,
    const char *name,
    FILE *file,
    size_t markov_length);

#ifdef __cplusplus
}
#endif
