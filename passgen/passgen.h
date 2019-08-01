/// @file passgen.h
/// @author Patrick M. Elsen <pelsen@xfbs.net>
/// @license MIT
///
/// Main functions of the passgen binary, used to parse command line arguments
/// and run the tool.

#pragma once
#include <stdlib.h>

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
} passgen_error;

/// Type used to encode a pattern preset.
typedef struct pattern_preset {
  /// The name that is specified on the command line.
  const char *name;
  /// The format of this preset.
  const char *format;
} pattern_preset;

/// List of pattern presets.
extern pattern_preset pattern_presets[];

/// Options parsed from the command line.
typedef struct {
  /// How many passwords to generate.
  size_t amount;
  /// The format to use when generating passwords.
  const char *format;
} passgen_opts;

/// Print usage (help text) using @p executable as the name of the program.
///
/// @param executable Name of the program, typically `argv[0]`.
void passgen_usage(const char *executable);

/// Print the version of the program.
void passgen_show_version(void);

/// Exit the program early using the specified error and data.
void passgen_bail(passgen_error error, void *data);

/// Parse the options passed on the command line, @p argc and @p argv, and
/// return the parsed result.
passgen_opts passgen_optparse(int argc, char *argv[]);

/// Run passgen with @p opts.
void passgen_run(passgen_opts opts);
