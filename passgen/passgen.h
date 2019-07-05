#pragma once
#include <stdlib.h>

typedef enum {
  PASSGEN_ERROR_NONE,
  PASSGEN_ERROR_HELP,
  PASSGEN_ERROR_MULTIPLE_FORMATS,
  PASSGEN_ERROR_RANDOM_ALLOC,
  PASSGEN_ERROR_PATTERN_PARSE,
  PASSGEN_ERROR_ALLOC,
} passgen_error;

typedef struct {
  size_t amount;
  const char *format;
} passgen_opts;

void passgen_usage(const char *executable);
void passgen_bail(passgen_error error, void *data);
passgen_opts passgen_optparse(int argc, char *argv[]);
void passgen_run(passgen_opts opts);
