/// @file passgen.h
/// @author Patrick M. Elsen <pelsen@xfbs.net>
/// @license MIT
///
/// Main functions of the passgen binary, used to parse command line arguments
/// and run the tool.

#pragma once
#include <stdbool.h>
#include <stdlib.h>
#include <passgen/util/random.h>
#include <passgen/util/hashmap.h>
#include "passgen/pattern/pattern.h"

/// The kinds of errors that cause the binary to exit early and with a nonzero
/// return code.
typedef enum {
    PASSGEN_ERROR_NONE,
    PASSGEN_ERROR_HELP,
    PASSGEN_ERROR_VERSION,
    PASSGEN_ERROR_MULTIPLE_FORMATS,
    PASSGEN_ERROR_RANDOM_ALLOC,
    PASSGEN_ERROR_PATTERN_PARSE,
    PASSGEN_ERROR_ALLOC,
    PASSGEN_ERROR_ILLEGAL_AMOUNT,
    PASSGEN_ERROR_ILLEGAL_MARKOV_LENGTH,
} passgen_cli_error;

/// Options parsed from the command line.
typedef struct {
    /// How many passwords to generate.
    size_t amount;
    /// The pattern to use when generating passwords.
    const char *pattern;
    /// The source of randomness to use.
    passgen_random *random;
    /// Separate output with null characters.
    bool null;
    /// Output as JSON.
    bool json;
    /// Parsing depth.
    size_t depth;
    /// Output complexity.
    bool complexity;
    /// Length of markov chain
    size_t markov_length;
    /// Wordlists that are to be loaded.
    passgen_hashmap wordlists;
    /// Presets to choose from.
    passgen_hashmap presets;
} passgen_cli_opts;

/// Initialize passgen_cli_opts to default values.
void passgen_cli_opts_init(passgen_cli_opts *opts);

/// Define a preset, with an argument like `name:value`.
int passgen_cli_opts_define_preset(passgen_cli_opts *opt, const char *arg);

/// Define a wordlist, with an argument like `name:path`.
int passgen_cli_opts_wordlist(passgen_cli_opts *opt, const char *input);

/// Parse a randomness definition, like `system` or `xor:2342322`
int passgen_cli_opts_random(passgen_cli_opts *opts, const char *random);

/// Load system and user config into options
int passgen_cli_opts_config(passgen_cli_opts *opts);

/// Load system config into options, if it exists
int passgen_cli_opts_config_system(passgen_cli_opts *opts);

/// Load user config into options, if it exists
int passgen_cli_opts_config_user(passgen_cli_opts *opts);

/// Load configuration options from file
int passgen_cli_opts_config_load(passgen_cli_opts *opts, FILE *file);

/// Load configuration options from string
int passgen_cli_opts_config_parse(passgen_cli_opts *opts, char *data);

/// Print usage (help text) using @p executable as the name of the program.
///
/// @param executable Name of the program, typically `argv[0]`.
void passgen_cli_usage(const char *executable);

/// Print the version of the program.
void passgen_cli_show_version(void);

/// Exit the program early using the specified error and data.
void passgen_cli_bail(passgen_cli_error error, void *data);

/// Parse the options passed on the command line, @p argc and @p argv, and
/// return the parsed result.
int passgen_cli_opts_parse(passgen_cli_opts *opts, int argc, char *argv[]);

/// Run passgen with @p opts.
void passgen_cli_run(passgen_cli_opts opts);

/// Generate passgen passwords.
void passgen_cli_generate(passgen_cli_opts opts, struct passgen_pattern *pattern);

/// Free data from passgen ops.
void passgen_cli_opts_free(passgen_cli_opts *opts);

#ifdef PASSGEN_SECCOMP
/// Initialize seccomp filters
void passgen_cli_seccomp_init();
#endif
