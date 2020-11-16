#include "passgen/wordlist.h"
#include "passgen/assert.h"
#include <stdlib.h>

void dump(const char *filename) {
  FILE *file = fopen(filename, "r");
  assert(file);

  passgen_wordlist_t wordlist;
  passgen_wordlist_load(&wordlist, file);
  fclose(file);

  printf("size = %zu\n", wordlist.size);
  printf("count = %zu\n", wordlist.count);

  for(size_t i = 0; i < wordlist.count; i++) {
    printf("%s\n", wordlist.words[i]);
  }

  passgen_wordlist_free(&wordlist);
}

int main(int argc, char *argv[]) {
  dump(argv[1]);

  return EXIT_SUCCESS;
}

