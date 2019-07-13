#pragma once
#include <stdlib.h>

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

typedef struct pattern_preset {
  const char *name;
  const char *format;
} pattern_preset;

extern pattern_preset pattern_presets[];

typedef struct {
  size_t amount;
  const char *format;
} passgen_opts;

void passgen_usage(const char *executable);
void passgen_bail(passgen_error error, void *data);
passgen_opts passgen_optparse(int argc, char *argv[]);
void passgen_run(passgen_opts opts);
