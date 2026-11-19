/// @file passgen.h
/// @author Patrick M. Elsen <pelsen@xfbs.net>
/// @license MIT
///
/// Main functions of the passgen binary, used to parse command line arguments
/// and run the tool.

#pragma once
#include <passgen/passgen.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/// Returns from the function if the statement does not result in zero.
#define try(statement)       \
    do {                     \
        int ret = statement; \
        if(ret != 0) {       \
            return ret;      \
        }                    \
    } while(0)

/// The kinds of errors that cause the binary to exit early and with a nonzero
/// return code. These are the exit statuses of the command-line tool, so they
/// should not be reordered. They are not the errors of the library, which are
/// in `passgen/error.h`.
typedef enum {
    PASSGEN_CLI_ERROR_NONE,
    PASSGEN_CLI_ERROR_MULTIPLE_FORMATS,
    PASSGEN_CLI_ERROR_RANDOM_ALLOC,
    PASSGEN_CLI_ERROR_PATTERN_PARSE,
    PASSGEN_CLI_ERROR_ALLOC,
    PASSGEN_CLI_ERROR_ILLEGAL_AMOUNT,
    // was an invalid --markov-length, which no longer exists. the length is
    // part of the pattern now, as in \m{name:2}.
    PASSGEN_CLI_ERROR_UNUSED_6,
    PASSGEN_CLI_ERROR_UNRECOGNIZED,
    PASSGEN_CLI_ERROR_PRESET_NOT_FOUND,
    PASSGEN_CLI_ERROR_GENERATE,
    PASSGEN_CLI_ERROR_WORDLIST,
    // was an invalid --depth, which no longer exists.
    PASSGEN_CLI_ERROR_UNUSED_11,
    PASSGEN_CLI_ERROR_MASTER_CONFLICT,
    PASSGEN_CLI_ERROR_ILLEGAL_STEPS,
    PASSGEN_CLI_ERROR_CONFIG,
    PASSGEN_CLI_ERROR_MASTER,

    PASSGEN_CLI_SHOW_HELP,
    PASSGEN_CLI_SHOW_VERSION,
} passgen_cli_error;

/// A preset or a wordlist which is defined on the command line. These are
/// added after the configuration is loaded, so that they replace what it
/// defines.
typedef struct {
    /// The option it was given with: `P` for a preset, `w` for a wordlist.
    char option;
    /// Its argument, as `name:pattern` or `name:path`.
    const char *argument;
} passgen_cli_definition;

/// A master passphrase which is defined in the configuration, as
/// `[master.name]`.
typedef struct {
    /// Its name.
    char *name;
    /// The version of how passphrases are derived from it.
    unsigned version;
    /// The level, which is how expensive deriving the master key is.
    unsigned level;
    /// A prefix of its hash, in lowercase hex, or NULL if there is none.
    char *hash;
    /// The seed, which may be empty.
    uint8_t seed[64];
    /// The length of the seed in bytes.
    size_t seed_size;
} passgen_cli_master;

/// Options parsed from the command line.
typedef struct {
    /// How many passwords to generate.
    size_t amount;
    /// The pattern to use when generating passwords.
    const char *pattern;
    /// The preset to use when generating passwords, which is looked up once
    /// the configuration is loaded.
    const char *preset;
    /// List the presets, instead of generating.
    bool list;
    /// Presets and wordlists defined on the command line, in order.
    passgen_cli_definition *definitions;
    /// How many presets and wordlists were defined on the command line.
    size_t definitions_len;
    /// Configuration files given on the command line, which are loaded
    /// instead of the usual ones.
    const char **configs;
    /// How many configuration files were given on the command line.
    size_t configs_len;
    /// Separate output with null characters.
    bool null;
    /// Output as JSON.
    bool json;
    /// Collect entropy
    bool entropy;

#ifdef PASSGEN_MONOCYPHER
    /// Name of the master passphrase to use for master passphrase mode.
    const char *master;
    /// Domain for master passphrase mode.
    const char *master_domain;
    /// User for master passphrase mode.
    const char *master_user;
    /// Name of a master passphrase to set up, instead of generating.
    const char *master_setup;
    /// Level of the master passphrase to set up.
    unsigned master_setup_level;
    /// Whether to generate a seed for the master passphrase to set up.
    bool master_setup_seed;
    /// Whether the version of the master passphrase to set up was given,
    /// which can only be the latest one.
    bool master_setup_version;
#endif
    /// Master passphrases defined in the configuration. These are read even
    /// without master passphrase mode, so that the configuration is the same.
    passgen_cli_master *masters;
    /// How many master passphrases are defined in the configuration.
    size_t masters_len;

    /// Source of randomness to generate from. This is what the environment
    /// uses, but it is owned by the options.
    passgen_random *random;
    /// Environment for passgen
    passgen_env *env;
} passgen_cli_opts;

/// Initialize passgen_cli_opts to default values.
void passgen_cli_opts_init(passgen_cli_opts *opts);

/// Define a preset, with an argument like `name:value`.
int passgen_cli_opts_preset(passgen_cli_opts *opts, const char *arg);

/// Define a wordlist, with an argument like `name:path`.
int passgen_cli_opts_wordlist(passgen_cli_opts *opt, const char *input);

/// Parse a randomness definition, like `system` or `xorshift:2342322`
int passgen_cli_opts_random(passgen_cli_opts *opts, const char *random);

/// Load the configuration files: the ones given with `--config`, or else
/// the ones in `PASSGEN_CONFIG`, or else the usual ones.
int passgen_cli_opts_config(passgen_cli_opts *opts);

/// Load a configuration file. One which does not exist is skipped, unless it
/// is required.
int passgen_cli_opts_config_file(
    passgen_cli_opts *opts,
    const char *path,
    bool required);

/// Load the configuration, add the presets and wordlists from the command
/// line, and find the preset to use. This is done after parsing the options,
/// which say which configuration to load.
int passgen_cli_opts_load(passgen_cli_opts *opts);

/// Print usage (help text) using @p executable as the name of the program.
///
/// @param executable Name of the program, typically `argv[0]`.
void passgen_cli_usage(const char *executable);

/// Print the version of the program.
void passgen_cli_show_version(void);

/// Exit the program early using the specified error and data.
int passgen_cli_bail(passgen_cli_error error, const void *data);

/// Parse the options passed on the command line, @p argc and @p argv, and
/// return the parsed result.
int passgen_cli_opts_parse(passgen_cli_opts *opts, int argc, char *argv[]);

/// Run passgen with @p opts.
int passgen_cli_run(passgen_cli_opts opts);

/// Generate passgen passwords.
int passgen_cli_generate(passgen_cli_opts opts, passgen_pattern *pattern);

/// Free data from passgen ops.
void passgen_cli_opts_free(passgen_cli_opts *opts);

#ifdef PASSGEN_SECCOMP
/// Initialize seccomp filters
void passgen_cli_seccomp_init(void);
#endif
