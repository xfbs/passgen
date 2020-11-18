#include "passgen/wordlist.h"
#include "tests.h"
#include <string.h>

const char example_wordlist[] = "abacus\nbrother\nculling";
size_t example_wordlist_size = sizeof(example_wordlist);

test_result test_wordlist_load(void) {
     FILE *file =
         fmemopen((void *) example_wordlist, example_wordlist_size, "r");
     assert(file);

     passgen_wordlist_t wordlist;
     passgen_wordlist_load(&wordlist, file);
     fclose(file);

     assert(wordlist.count == 3);
     assert(0 == strcmp(wordlist.words[0], "abacus"));
     assert(0 == strcmp(wordlist.words[1], "brother"));
     assert(0 == strcmp(wordlist.words[2], "culling"));

     passgen_wordlist_free(&wordlist);

     return test_ok;
}

test_result test_wordlist_random(void) {
     FILE *file =
         fmemopen((void *) example_wordlist, example_wordlist_size, "r");
     assert(file);

     passgen_wordlist_t wordlist;
     passgen_wordlist_load(&wordlist, file);
     fclose(file);

     passgen_random_t random;
     passgen_random_open(&random);

     for(size_t i = 0; i < wordlist.count; i++) {
          const char *word = passgen_wordlist_random(&wordlist, &random);

          // make sure the word exists in the word list
          bool found = false;
          for(size_t j = 0; j < wordlist.count; j++) {
               if(word == wordlist.words[j]) {
                    found = true;
                    break;
               }
          }
          assert(found);
     }

     passgen_wordlist_free(&wordlist);
     passgen_random_close(&random);

     return test_ok;
}
