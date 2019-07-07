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


passgen_opts passgen_optparse(int argc, char *argv[]) {
  passgen_opts opts = {
      .format = NULL,
      .amount = 1,
  };

  enum presets {
    PRESET_NONE,
    PRESET_APPLE,
    PRESET_APPLE_OLD,
  };

  static int preset;
  const char *short_opts = "a:p:h";

  static struct option long_opts[] = {
    {"amount", required_argument, NULL, 'a'},
    {"help", no_argument, NULL, 'h'},
    {"pattern", no_argument, NULL, 'p'},
    {"apple", no_argument, &preset, PRESET_APPLE},
    {"apple-old", no_argument, &preset, PRESET_APPLE_OLD},
    {NULL, no_argument, NULL, 0}
  };

  while(true) {
    int opt = getopt_long(argc, argv, short_opts, long_opts, NULL);

    if(opt < 0) break;

    switch(opt) {
      case 0:
        break;
      case 'h':
        bail(HELP, argv[0]);
        break;
      case 'a':
        opt = atoi(optarg);
        if(opt < 1) {
          bail(ILLEGAL_AMOUNT, &opt);
        }
        opts.amount = opt;
        break;
      case 'p':
        opts.format = optarg;
        break;
      case '?':
      default:
        fprintf(stderr, "Error unrecognised: %s\n", argv[optind]);
        bail(HELP, argv[0]);
        break;
    }
  }

  if(preset && opts.format) {
    bail(MULTIPLE_FORMATS, opts.format);
  }

  switch(preset) {
    case PRESET_APPLE:
      opts.format = "[a-zA-Z0-9]{3}(-[a-zA-Z0-9]{3}){3}";
      break;
    case PRESET_APPLE_OLD:
      opts.format = "[a-zA-Z0-9]{5}(-[a-zA-Z0-9]{5}){2}";
      break;
  }

  if(optind < argc) {
    if(opts.format || (argc - optind) > 1) {
      bail(MULTIPLE_FORMATS, (void *)argv[optind]);
    } else {
      opts.format = argv[optind];
    }
  }

  if(!opts.format) {
    bail(HELP, argv[0]);
  }

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
    case PASSGEN_ERROR_ILLEGAL_AMOUNT:
      printf("Error: illegal amount entered (%i).\n", *((int *) data));
      exit(-5);
    default:
      printf("Error: unknown error.\n");
      exit(-100);
  }
}
