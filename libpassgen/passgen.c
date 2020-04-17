#include "passgen/passgen.h"

#include <getopt.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "passgen/data/array.h"
#include "passgen/memory.h"
#include "passgen/pattern.h"
#include "passgen/pronounceable.h"
#include "passgen/random.h"
#include "passgen/version.h"

#define bail(kind, data) passgen_bail(PASSGEN_ERROR_##kind, (void *)data)

void passgen_run(passgen_opts opts) {
  passgen_mem_accounting_t acc = passgen_mem_accounting_new(NULL);
  passgen_mem_t mem = passgen_mem_accounting(&acc);
  pattern_result_t result;

  // initialize source of random numbers
  passgen_random_t random;
  if(!passgen_random_open(&random)) bail(RANDOM_ALLOC, NULL);

  // parse format
  pattern_t pattern;
  result = pattern_parse(&pattern, opts.format, opts.depth, &mem);

  if(!result.ok) {
    passgen_random_close(&random);
    pattern_error_show(result, opts.format);
    bail(PATTERN_PARSE, opts.format);
  }

  // allocate some space for pass.
  // size_t pass_len = pattern_maxlen(pattern);
  size_t pass_len = 256;
  char *pass = malloc(pass_len + 1);
  if(!pass) {
    passgen_random_close(&random);
    bail(ALLOC, NULL);
  }

  char sep = '\n';
  if(opts.null) {
    sep = '\0';
  }

  for(size_t i = 0; i < opts.amount; ++i) {
    pattern_env_t env = {
        .find_complexity = opts.complexity,
        .pronounceable_limit = 1000,
        .pronounceable_type = PASSGEN_PRONOUNCEABLE_ENGLISH,
    };

    // get a NULL-terminated, random pass.
    size_t written =
        pattern_passgen_random_fill(&pattern, &random, &env, pass, pass_len);
    pass[written] = '\0';

    if(opts.complexity) {
      fprintf(stderr, "entropy: %lf bits\n", log(env.complexity) / log(2));
    }

    if(i == 0) {
      printf("%s", pass);
    } else {
      printf("%c%s", sep, pass);
    }
  }

  printf("\n");
  free(pass);
  pattern_free(&pattern);
  passgen_mem_accounting_cleanup(&acc);
  passgen_random_close(&random);
}

passgen_opts passgen_optparse(int argc, char *argv[]) {
  passgen_opts opts = {
      .format = NULL,
      .amount = 1,
      .depth = 100,
      .null = false,
      .complexity = false,
  };

  const char *short_opts = "a:p:d:czhv";
  const char *preset = NULL;

  // clang-format off
    static struct option long_opts[] = {
        {"amount", required_argument, NULL, 'a'},
        {"help", no_argument, NULL, 'h'},
        {"preset", required_argument, NULL, 'p'},
        {"depth", required_argument, NULL, 'd'},
        {"version", no_argument, NULL, 'v'},
        {"null", no_argument, NULL, 'z'},
        {"complexity", no_argument, NULL, 'c'},
        {NULL, no_argument, NULL, 0}
    };
  // clang-format on

  while(true) {
    int opt = getopt_long(argc, argv, short_opts, long_opts, NULL);

    if(opt < 0) break;

    switch(opt) {
      case 0: break;
      case 'h': bail(HELP, argv[0]); break;
      case 'a':
        opt = atoi(optarg);
        if(opt < 1) {
          bail(ILLEGAL_AMOUNT, &opt);
        }
        opts.amount = opt;
        break;
      case 'p': preset = optarg; break;
      case 'd':
        opt = atoi(optarg);
        opts.depth = opt;
        break;
      case 'z': opts.null = true; break;
      case 'c': opts.complexity = true; break;
      case 'v': bail(VERSION, NULL);
      case '?':
      default:
        fprintf(stderr, "Error unrecognised: %s\n", argv[optind]);
        bail(HELP, argv[0]);
        break;
    }
  }

  // can't have both a preset and a format at the same time.
  if(preset && opts.format) {
    bail(MULTIPLE_FORMATS, opts.format);
  }

  // if a preset was given, parse it.
  if(preset) {
    for(size_t i = 0; pattern_presets[i].name; ++i) {
      if(0 == strcmp(pattern_presets[i].name, preset)) {
        opts.format = pattern_presets[i].format;
        break;
      }
    }
  }

  // parse a given format, making sure we don't have multiple."
  if(optind < argc) {
    if(opts.format || (argc - optind) > 1) {
      bail(MULTIPLE_FORMATS, (void *)argv[optind]);
    } else {
      opts.format = argv[optind];
    }
  }

  // if no format or preset was given, show help.
  if(!opts.format) {
    bail(HELP, argv[0]);
  }

  return opts;
}

void passgen_usage(const char *executable) {
  fprintf(
      stderr,
      "passgen version %s\n"
      "Generate passwords from a regex-like pattern.\n"
      "Usage: %s [OPTIONS] [PATTERN]\n\n"
      "PATTERN is a regex-like string describing the password.\n"
      "    abc|def           Matches string 'abc' or 'def' (choice).\n"
      "    [a-cf]            Matches character 'a', 'b', 'c', and 'f' "
      "(range).\n"
      "    (abc)             Matches strings 'abc' (group).\n"
      "    [a-c]{2,3}        Matches between 2 and 3 repetition of element "
      "(repeat).\n"
      "\n"
      "OPTIONS\n"
      "    -a, --amount      The amount of passwords\n"
      "    -z, --null        Use NUL instead of newline to separate passes.\n"
      "    -h, --help        Show this help information\n"
      "    -p, --preset      Use the given preset.\n"
      "    -v, --version     Show the version of this build.\n"
      "    -d, --depth       Limit the parsing depth.\n"
      "    -c, --complexity  Output complexity for each password.\n"
      "\n"
      "PRESETS\n"
      "    apple1            Generate passwords like 'oKC-T37-Dew-Qyn'.\n"
      "    apple2            Generate passwords like 'mHXr4X-CiK4w6-hbjF7T'.\n",
      passgen_version_str(),
      executable);
}

void passgen_show_version(void) {
  fprintf(stderr, "passgen, version %s\n", passgen_version_str());

  if(passgen_is_debug()) {
    fprintf(stderr, "debug build\n");
  } else {
    fprintf(stderr, "release build\n");
  }
}

void passgen_bail(passgen_error error, void *data) {
  switch(error) {
    case PASSGEN_ERROR_HELP: passgen_usage(data); exit(-1);
    case PASSGEN_ERROR_VERSION: passgen_show_version(); exit(-1);
    case PASSGEN_ERROR_MULTIPLE_FORMATS:
      printf("Error: multiple formats specified (%s).\n", (const char *)data);
      exit(-2);
    case PASSGEN_ERROR_RANDOM_ALLOC:
      printf("Error: couldn't open random object.\n");
      exit(-3);
    case PASSGEN_ERROR_PATTERN_PARSE:
      // printf("Error: couldn't parse pattern '%s'.\n", (const char *)data);
      exit(-4);
    case PASSGEN_ERROR_ILLEGAL_AMOUNT:
      printf("Error: illegal amount entered (%i).\n", *((int *)data));
      exit(-5);
    default: printf("Error: unknown error.\n"); exit(-100);
  }
}
