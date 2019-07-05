#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <assert.h>
#include "random.h"
#include "pattern.h"

typedef enum {
  PASSGEN_ERROR_MULTIPLE_FORMATS,
  PASSGEN_ERROR_RANDOM_ALLOC,
  PASSGEN_ERROR_PATTERN_PARSE,
  PASSGEN_ERROR_ALLOC,
} passgen_error;

void usage(const char *executable);
void passgen_bail(passgen_error error, void *data);
#define bail(kind, data) passgen_bail(PASSGEN_ERROR_ ## kind, (void *) data)

int main(int argc, char *argv[])
{
    // command line flags
    int opt_amount = -1;
    const char *opt_format = NULL;

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
            usage(argv[0]);
            return 0;
        } else if((strcmp(argv[i], "-a") == 0) || (strcmp(argv[i], "--amount") == 0)) {
            if((i+1) < argc) {
                opt_amount = atoi(argv[i+1]);
                i++;
            } else {
                usage(argv[0]);
                return -1;
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

    // initialize source of random numbers
    random_t *random = random_new();
    if(!random) bail(RANDOM_ALLOC, NULL);

    // parse format
    const char *parse_pos = opt_format;
    pattern_t *pattern = pattern_parse(&parse_pos);
    if(!pattern) {
      random_close(random);
      bail(PATTERN_PARSE, opt_format);
    }

    // allocate some space for pass.
    //size_t pass_len = pattern_maxlen(pattern);
    size_t pass_len = 256;
    char *pass = malloc(pass_len + 1);
    if(!pass) {
      random_close(random);
      bail(ALLOC, NULL);
    }

    for(size_t i = 0; i < opt_amount; ++i) {
      // get a NULL-terminated, random pass.
      size_t written = pattern_random_fill(pattern, random, pass, pass_len);
      pass[written] = '\0';

      printf((i == 0) ? "%s" : "\t%s", pass);
    }

    printf("\n");
    free(pass);
    pattern_free(pattern);
    random_close(random);

    return 0;
}

void usage(const char *executable) {
    printf("Usage: %s [OPTIONS] [FORMAT]\n", executable);
    printf("  -h, --help         Show this usage information\n");
    printf("  -a, --amount       The amount of passwords\n");
}

void passgen_bail(passgen_error error, void *data) {
  switch(error) {
    case PASSGEN_ERROR_MULTIPLE_FORMATS:
      printf("Error: multiple formats specified (%s).\n", data);
      exit(-1);
    case PASSGEN_ERROR_RANDOM_ALLOC:
      printf("Error: couldn't open random object.\n");
      exit(-2);
    case PASSGEN_ERROR_PATTERN_PARSE:
      printf("Error: couldn't parse pattern (%s).\n", data);
      exit(-3);
    default:
      printf("Error: unknown error.\n");
      exit(-100);
  }
}
