#include "passgen/markov.h"
#include "passgen/markov_data.h"
#include "tests.h"
#define SEED 328543091702ULL

test_result check_markov3(const struct passgen_markov3 *data3) {
     assert(data3->list);
     assert(data3->list_len);
     assert(data3->codepoint == 0);

     int32_t data2_last_codepoint = 0;
     for(size_t i3 = 0; i3 < data3->list_len; i3++) {
          const struct passgen_markov2 *data2 = &data3->list[i3];

          if(i3 == 0) {
               // first codepoint should be null (must exist)
               assert(data2->codepoint == 0);
          } else {
               // codepoints have to be sorted
               assert(data2->codepoint > data2_last_codepoint);
          }

          data2_last_codepoint = data2->codepoint;

          int32_t data1_last_codepoint = 0;
          for(size_t i2 = 0; i2 < data2->list_len; i2++) {
               const struct passgen_markov1 *data1 = &data2->list[i2];

               if(i2 == 0) {
                    if(i3 == 0) {
                         // there must be a null - null sequence in the chain
                         assert(data1->codepoint == 0);
                    }
               } else {
                    // codepoints have to be sorted
                    assert(data1->codepoint > data1_last_codepoint);
               }

               data1_last_codepoint = data1->codepoint;

               size_t frequency_sum = 0;
               size_t prev_frequency = 0;
               for(size_t i1 = 0; i1 < data1->list_len; i1++) {
                    const struct passgen_markov0 *data0 = &data1->list[i1];
                    assert(data0->frequency);

                    // make sure it's sorted by frequency in descending order
                    if(i1) {
                         assert(data0->frequency <= prev_frequency);
                    }

                    frequency_sum += data0->frequency;
                    prev_frequency = data0->frequency;
               }

               assert(frequency_sum == data1->frequency_sum);
          }
     }

     return test_ok;
}

test_result test_markov_english(void) {
     return check_markov3(&passgen_markov_english);
}

test_result test_markov_latin(void) {
     return check_markov3(&passgen_markov_latin);
}

test_result test_markov_german(void) {
     return check_markov3(&passgen_markov_german);
}

test_result test_markov2_find(void) {
     const struct passgen_markov2 *list;

     list = passgen_markov3_find(&passgen_markov_english, '+');
     assert(list == NULL);

     list = passgen_markov3_find(&passgen_markov_english, 0);
     assert(list != NULL);
     assert(list->codepoint == 0);

     list = passgen_markov3_find(&passgen_markov_english, 'a');
     assert(list != NULL);
     assert(list->codepoint == 'a');

     return test_ok;
}

test_result test_markov3_find(void) {
     const struct passgen_markov1 *list;
     const struct passgen_markov2 *list2;

     list2 = passgen_markov3_find(&passgen_markov_english, 0);
     assert(list2);

     list = passgen_markov2_find(list2, '+');
     assert(list == NULL);

     list = passgen_markov2_find(list2, 0);
     assert(list != NULL);
     assert(list->codepoint == 0);

     list = passgen_markov2_find(list2, 'a');
     assert(list != NULL);
     assert(list->codepoint == 'a');

     return test_ok;
}

test_result test_markov1_find(void) {
     const struct passgen_markov0 *choice;
     const struct passgen_markov1 *list;
     const struct passgen_markov2 *list2;

     list2 = passgen_markov3_find(&passgen_markov_english, 0);
     assert(list2);

     list = passgen_markov2_find(list2, 0);
     assert(list != NULL);
     assert(list->codepoint == 0);

     choice = passgen_markov1_find(list, list->frequency_sum);
     assert(choice == NULL);

     choice = passgen_markov1_find(list, 0);
     assert(choice != NULL);
     assert(choice->codepoint == 's');

     choice = passgen_markov1_find(list, list->frequency_sum - 1);
     assert(choice != NULL);
     assert(choice->codepoint == 'x');

     return test_ok;
}
