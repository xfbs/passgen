#include "passgen/pronounceable.h"

#include <stdlib.h>
#include <string.h>

#include "passgen/assert.h"
#include "passgen/markov.h"
#include "passgen/markov_data.h"

const struct passgen_markov3 *markov_pronounceable_all[] = {
    NULL};

passgen_pronounceable_map_t passgen_pronounceable_map[] = {
    {NULL, 0},
};

enum passgen_pronounceable_type
passgen_pronounceable_lookup(size_t length, const char *name) {
     for(size_t i = 0; i < PASSGEN_PRONOUNCEABLE_LAST; i++) {
          if(0 == strncmp(passgen_pronounceable_map[i].name, name, length)) {
               return passgen_pronounceable_map[i].type;
          }
     }

     return PASSGEN_PRONOUNCEABLE_LAST;
}

size_t passgen_pronounceable(
    enum passgen_pronounceable_type type,
    passgen_random_t *rand,
    int32_t *buf,
    size_t len) {
     assert(type < PASSGEN_PRONOUNCEABLE_LAST);

     /* get list */
     const struct passgen_markov3 *list = markov_pronounceable_all[type];
     assert(list);

     return passgen_markov2_generate(list, rand, buf, len);
}

size_t passgen_pronounceable_len(
    enum passgen_pronounceable_type type,
    passgen_random_t *rand,
    int32_t *buf,
    size_t min,
    size_t max,
    size_t tries) {
     assert(type < PASSGEN_PRONOUNCEABLE_LAST);

     /* get list */
     const struct passgen_markov3 *list = markov_pronounceable_all[type];
     assert(list);

     /* try tries times (or infinite times if tries is 0) to fill buffer up with
      * a word. return 0 on success. */
     for(size_t i = 0; !tries || i < tries; i++) {
          int32_t p1 = 0, p2 = 0;

          /* position in the output buffer and codepoints is not the same thing.
           */
          size_t pos = 0;

          do {
               const struct passgen_markov0 *choice;
               choice = passgen_markov2_choose(list, rand, p1, p2);
               assert(choice);

               /* advante to next pair of previour */
               p1 = p2;
               p2 = choice->codepoint;

               /* make sure we don't write too much. */
               if(pos == max) {
                    pos++;
                    break;
               }

               /* save codepoint in buffer */
               buf[pos++] = choice->codepoint;
          } while(p2);

          /* if this isn't finished, or is too short, skip */
          if(p2 || pos <= min) {
               continue;
          }

          return pos - 1;
     }

     return 0;
}
