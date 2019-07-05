#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <assert.h>
#include "random.h"
#include "pattern.h"

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
#define bail(kind, data) passgen_bail(PASSGEN_ERROR_ ## kind, (void *) data)
passgen_opts passgen_optparse(int argc, char *argv[]);
void passgen_run(passgen_opts opts);

int main(int argc, char *argv[]) {
    passgen_opts opts = passgen_optparse(argc, argv);
    passgen_run(opts);

    return EXIT_SUCCESS;
}

void passgen_run(passgen_opts opts) {
    // initialize source of random numbers
    random_t *random = random_new();
    if(!random) bail(RANDOM_ALLOC, NULL);

    // parse format
    const char *parse_pos = opts.format;
    pattern_t *pattern = pattern_parse(&parse_pos);
    if(!pattern) {
      random_close(random);
      bail(PATTERN_PARSE, opts.format);
    }

    // allocate some space for pass.
    //size_t pass_len = pattern_maxlen(pattern);
    size_t pass_len = 256;
    char *pass = malloc(pass_len + 1);
    if(!pass) {
      random_close(random);
      bail(ALLOC, NULL);
    }

    for(size_t i = 0; i < opts.amount; ++i) {
      // get a NULL-terminated, random pass.
      size_t written = pattern_random_fill(pattern, random, pass, pass_len);
      pass[written] = '\0';

      printf((i == 0) ? "%s" : "\t%s", pass);
    }

    printf("\n");
    free(pass);
    pattern_free(pattern);
    random_close(random);
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
    for(int i = 1; i < argc; i++) {
        if(flags) {
            if(opt_format) {
                bail(MULTIPLE_FORMATS, (void *) argv[i]);
            } else {
                opt_format = argv[i];
            }
        } else if((strcmp(argv[i], "-h") == 0) || (strcmp(argv[i], "--help") == 0)) {
            bail(HELP, argv[0]);
        } else if((strcmp(argv[i], "-a") == 0) || (strcmp(argv[i], "--amount") == 0)) {
            if((i+1) < argc) {
                opt_amount = atoi(argv[i+1]);
                i++;
            } else {
                bail(HELP, argv[0]);
            }
        } else if(0 == strcmp(argv[i], "--")) {
            flags = true;
        } else {
            opt_format = argv[i];
            flags = true;
        }
    }

    // if the amonut is set to something illegal, rectify it
    if(opt_amount <= 0) {
        opt_amount = 1;
    }

    opts.amount = opt_amount;
    opts.format = opt_format;

    return opts;
}

void passgen_usage(const char *executable) {
    printf("Usage: %s [OPTIONS] [FORMAT]\n", executable);
    printf("  -h, --help         Show this passgen_usage information\n");
    printf("  -a, --amount       The amount of passwords\n");
}

void passgen_bail(passgen_error error, void *data) {
  switch(error) {
    case PASSGEN_ERROR_HELP:
      passgen_usage(data);
      exit(-1);
    case PASSGEN_ERROR_MULTIPLE_FORMATS:
      printf("Error: multiple formats specified (%s).\n", data);
      exit(-2);
    case PASSGEN_ERROR_RANDOM_ALLOC:
      printf("Error: couldn't open random object.\n");
      exit(-3);
    case PASSGEN_ERROR_PATTERN_PARSE:
      printf("Error: couldn't parse pattern (%s).\n", data);
      exit(-4);
    default:
      printf("Error: unknown error.\n");
      exit(-100);
  }
}
