#include <stdio.h>
#include "passgen/pattern.h"

int main(int argc, char *argv[]) {
  (void)argc;

  if (argc == 1) {
    fprintf(stderr, "Usage: %s PATTERN\n", argv[0]);
    fprintf(stderr, "Counts out how many choices there are for a pattern.\n");
    exit(-1);
  }

  const char *pattern_str = argv[1];
  pattern_t *pattern = pattern_parse(&pattern_str);

  if (!pattern) {
    int error_pos = pattern_str - argv[1];
    fprintf(stderr, "Error parsing pattern: %s\n", argv[1]);
    fprintf(stderr, "%*c^", error_pos + 13, ' ');
    exit(-1);
  }

  if (pattern_str[0] != '\0') {
    int error_pos = pattern_str - argv[1];
    fprintf(stderr, "Error parsing pattern: '%s'\n", argv[1]);
    fprintf(stderr, "%*cerror likely here ^\n", error_pos + 6, ' ');
  }

  printf("%zi\n", pattern_choices(pattern));

  if (pattern) {
    pattern_free(pattern);
  }

  return 0;
}
