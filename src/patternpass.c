#include "pattern.h"
#include "random.h"
#include <stdio.h>

int main(int argc, char *argv[]) {
  const char *s = argv[1];
  pattern_t *pattern = pattern_parse(&s);
  random_t *rand = random_new();
  const char *p = pattern_random(pattern, rand);
  printf("%s\n", p);

  random_close(rand);
  pattern_free(pattern);
  free((void *)p);
  return 0;
}
