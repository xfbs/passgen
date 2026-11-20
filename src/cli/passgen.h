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
    PASSGEN_CLI_ERROR_ILLEGAL_OFFSET,

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

/// A login which is defined in the configuration, as `[[login]]`: which master
/// passphrase, preset or pattern and offset a domain and user use.
typedef struct {
    /// The domain.
    char *domain;
    /// The user, which may be empty.
    char *user;
    /// The name of the master passphrase, or NULL if there is only one.
    char *master;
    /// The preset to use, or NULL.
    char *preset;
    /// The pattern to use, or NULL.
    char *pattern;
    /// The offset.
    uint64_t offset;
    /// The configuration file which defines it.
    char *path;
} passgen_cli_login;

/// Options parsed from the command line.
typedef struct {
    /// How many passwords to generate.
    size_t amount;
    /// Which stream of the source of randomness the first one is generated
    /// from. The ones after it use the streams after it.
    uint64_t offset;
    /// Whether the offset was given, which needs a source with streams.
    bool offset_given;
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
    /// Whether to set up a master passphrase, instead of generating.
    bool master_setup_given;
    /// Name of the master passphrase to set up, or NULL to ask for it.
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
    /// Logins defined in the configuration. These are read even without
    /// master passphrase mode, so that the configuration is the same.
    passgen_cli_login *logins;
    /// How many logins are defined in the configuration.
    size_t logins_len;

    /// Source of randomness to generate from. This is what the environment
    /// uses, but it is owned by the options.
    passgen_random *random;
    /// Environment for passgen
    passgen_env *env;
} passgen_cli_opts;

// options.c

/// Initialize passgen_cli_opts to default values.
void passgen_cli_opts_init(passgen_cli_opts *opts);

/// Define a preset, with an argument like `name:value`.
int passgen_cli_opts_preset(passgen_cli_opts *opts, const char *arg);

/// Define a wordlist, with an argument like `name:path`.
int passgen_cli_opts_wordlist(passgen_cli_opts *opt, const char *input);

/// Parse a randomness definition, like `system` or `xorshift:2342322`
int passgen_cli_opts_random(passgen_cli_opts *opts, const char *random);

/// Parse the options passed on the command line, @p argc and @p argv, and
/// return the parsed result.
int passgen_cli_opts_parse(passgen_cli_opts *opts, int argc, char *argv[]);

/// Load the configuration, add the presets and wordlists from the command
/// line, and find the preset to use. This is done after parsing the options,
/// which say which configuration to load.
int passgen_cli_opts_load(passgen_cli_opts *opts);

/// Free data from passgen ops.
void passgen_cli_opts_free(passgen_cli_opts *opts);

/// Print usage (help text) using @p executable as the name of the program.
///
/// @param executable Name of the program, typically `argv[0]`.
void passgen_cli_usage(const char *executable);

/// Print the version of the program.
void passgen_cli_show_version(void);

/// Exit the program early using the specified error and data.
int passgen_cli_bail(passgen_cli_error error, const void *data);

// config.c

/// Load the configuration files: the ones given with `--config`, or else
/// the ones in `PASSGEN_CONFIG`, or else the usual ones.
int passgen_cli_opts_config(passgen_cli_opts *opts);

/// Load a configuration file. One which does not exist is skipped, unless it
/// is required.
int passgen_cli_opts_config_file(
    passgen_cli_opts *opts,
    const char *path,
    bool required);

/// Find a master passphrase from the configuration by its name, or NULL if
/// there is none with it.
passgen_cli_master *
passgen_cli_master_find(const passgen_cli_opts *opts, const char *name);

/// Free what a master passphrase from the configuration holds.
void passgen_cli_master_free(passgen_cli_master *master);

/// Free what a login from the configuration holds.
void passgen_cli_login_free(passgen_cli_login *login);

// passgen.c

/// Run passgen with @p opts.
int passgen_cli_run(passgen_cli_opts opts);

// output.c

/// Generate passgen passwords.
int passgen_cli_generate(passgen_cli_opts opts, passgen_pattern *pattern);

// color.c

/// Styles of what is printed to standard error.
typedef enum {
    /// The label of an error, and what marks where in a pattern it is.
    PASSGEN_CLI_STYLE_ERROR,
    /// Where something is, such as the pattern or the path of a wordlist.
    PASSGEN_CLI_STYLE_NOTE,
    /// Back to no style.
    PASSGEN_CLI_STYLE_RESET,
} passgen_cli_style_kind;

/// The escape sequence for a style, or an empty string if standard error is
/// not a terminal, or `NO_COLOR` is set.
const char *passgen_cli_style(passgen_cli_style_kind style);

/// Print an error to standard error: `error: ` (in color, if it is used),
/// followed by the message, which is a format for printf. It does not add a
/// newline.
#ifdef __GNUC__
__attribute__((format(printf, 1, 2)))
#endif
void passgen_cli_error_print(const char *format, ...);

#ifdef PASSGEN_MONOCYPHER
// master.c

/// Set up a master passphrase: ask for what was not given as options (at a
/// terminal), and for the master passphrase, and print what the
/// configuration needs to use it.
int passgen_cli_master_setup(const passgen_cli_opts *opts);

/// Ask for the master passphrase, check it against the hash in the
/// configuration (if there is one), and open the source of randomness for
/// it, for the domain and user.
int passgen_cli_master_open(
    const passgen_cli_opts *opts,
    passgen_random **random);

// login.c

/// Find the login for the domain (and user, if it is given), and fill in
/// what was not given on the command line: the master passphrase, the user,
/// the preset or pattern, and the offset.
int passgen_cli_login_apply(passgen_cli_opts *opts);
#endif

#ifdef PASSGEN_SECCOMP
// seccomp.c

/// Initialize seccomp filters
void passgen_cli_seccomp_init(void);
#endif
