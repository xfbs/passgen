#include "passgen/passgen.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include "passgen/pattern.h"
#include "passgen/random.h"
#define bail(kind, data) passgen_bail(PASSGEN_ERROR_##kind, (void *)data)

void passgen_run(passgen_opts opts) {
  // initialize source of random numbers
  random_t random;
  if (!random_open(&random)) bail(RANDOM_ALLOC, NULL);

  // parse format
  const char *parse_pos = opts.format;
  pattern_t *pattern = pattern_parse(&parse_pos);
  if (!pattern) {
    random_close(&random);
    bail(PATTERN_PARSE, opts.format);
  }

  if(*parse_pos != '\0') {
    random_close(&random);
    bail(PATTERN_PARSE, opts.format);
  }

  // allocate some space for pass.
  // size_t pass_len = pattern_maxlen(pattern);
  size_t pass_len = 256;
  char *pass = malloc(pass_len + 1);
  if (!pass) {
    random_close(&random);
    bail(ALLOC, NULL);
  }

  for (size_t i = 0; i < opts.amount; ++i) {
    // get a NULL-terminated, random pass.
    size_t written = pattern_random_fill(pattern, &random, pass, pass_len);
    pass[written] = '\0';

    printf((i == 0) ? "%s" : "\t%s", pass);
  }

  printf("\n");
  free(pass);
  pattern_free(pattern);
  random_close(&random);
}

bool is(const char *arg, const char *s, const char *o) {
  if(s && 0 == strcmp(arg, s)) return true;
  if(o && 0 == strcmp(arg, o)) return true;
  return false;
}

passgen_opts passgen_optparse(int argc, char *argv[]) {
  // command line flags
  int opt_amount = -1;
  const char *opt_format = NULL;
  passgen_opts opts = {
      .format = NULL,
      .amount = 1,
  };

  // parses the command line flags
  bool flags = false;
  for (int i = 1; i < argc; i++) {
    if (flags) {
      if (opt_format) {
        bail(MULTIPLE_FORMATS, (void *)argv[i]);
      } else {
        opt_format = argv[i];
      }
    } else if (is(argv[i], "-h", "--help")) {
      bail(HELP, argv[0]);
    } else if (is(argv[i], NULL, "--apple-old")) {
      opt_format = "[a-zA-Z0-9]{3}(-[a-zA-Z0-9]{3}){3}";
    } else if (is(argv[i], NULL, "--apple")) {
      opt_format = "[a-zA-Z0-9]{5}(-[a-zA-Z0-9]{5}){2}";
    } else if (is(argv[i], "-a", "--amount")) {
      if ((i + 1) < argc) {
        opt_amount = atoi(argv[i + 1]);
        i++;
      } else {
        bail(HELP, argv[0]);
      }
    } else if (0 == strcmp(argv[i], "--")) {
      flags = true;
    } else {
      opt_format = argv[i];
      flags = true;
    }
  }

  // if the amonut is set to something illegal, rectify it
  if (opt_amount <= 0) {
    opt_amount = 1;
  }

  if(!opt_format) {
    bail(HELP, argv[0]);
  }

  opts.amount = opt_amount;
  opts.format = opt_format;

  return opts;
}

void passgen_usage(const char *executable) {
  fprintf(stderr,
      "Usage: %s [OPTIONS] PATTERN|PRESET\n\n"
      "PATTERN is a regex-like string describing the password.\n"
      "  abc|def          Matches string 'abc' or 'def' (choice).\n"
      "  [a-cf]           Matches character 'a', 'b', 'c', and 'f' (range).\n"
      "  (abc)            Matches strings 'abc' (group).\n"
      "  [a-c]{2,3}       Matches between 2 and 3 repetition of element (repeat).\n"
      "\n"
      "OPTIONS\n"
      "  -h, --help       Show this help information\n"
      "  -a, --amount     The amount of passwords\n"
      "\n"
      "PRESETS\n"
      "  --apple          Generate passwords like Apple.\n"
      "  --apple-old      Generate passwords like Apple before Mojave.\n", executable);
}

void passgen_bail(passgen_error error, void *data) {
  switch (error) {
    case PASSGEN_ERROR_HELP:
      passgen_usage(data);
      exit(-1);
    case PASSGEN_ERROR_MULTIPLE_FORMATS:
      printf("Error: multiple formats specified (%s).\n", (const char *) data);
      exit(-2);
    case PASSGEN_ERROR_RANDOM_ALLOC:
      printf("Error: couldn't open random object.\n");
      exit(-3);
    case PASSGEN_ERROR_PATTERN_PARSE:
      printf("Error: couldn't parse pattern '%s'.\n", (const char *) data);
      exit(-4);
    default:
      printf("Error: unknown error.\n");
      exit(-100);
  }
}
