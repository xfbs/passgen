#include "tests.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

test_result test_ok = {.ok = true};

static inline bool strprefix(const char *prefix, const char *string) {
  for(size_t offset = 0; prefix[offset]; offset++) {
    if(prefix[offset] != string[offset]) {
      return false;
    }
  }

  return true;
}

static inline void memswap(void *a, void *b, size_t size) {
  char data[size];

  memcpy(data, a, size);
  memcpy(a, b, size);
  memcpy(b, data, size);
}

static inline void shuffle(test_entry *tests) {
  for(size_t i = 0; i < (tests_len * 2); i++) {
    size_t pos_a = rand() % tests_len;
    size_t pos_b = rand() % tests_len;

    memswap(&tests[pos_a], &tests[pos_b], sizeof(test_entry));
  }
}

int main(int argc, char *argv[]) {
  // initialise shitty pseudorandom number generator
  srand(time(NULL));

  if(getenv("SHUFFLE")) {
    shuffle(tests);
  }

  size_t failures = 0;
  size_t success = 0;
  if(argc > 1) {
    // go through the arguments.
    for(size_t r = 1; r < (size_t) argc; ++r) {
      // go through the tests.
      for(size_t i = 0; tests[i].name; ++i) {
        // find the test with the name that is the current argument.
        if(strprefix(argv[r], tests[i].name)) {
          // run the testl
          if(run(tests[i])) {
            success += 1;
          } else {
            failures += 1;
          }
        }
      }
    }
  } else {
    for(size_t i = 0; tests[i].name; ++i) {
      if(run(tests[i])) {
        success += 1;
      } else {
        failures += 1;
      }
    }
  }

  printf(
      "\033[1;34m=>\033[0m %zi/%zi tests passed.\n",
      success,
      success + failures);

  if(failures) {
    return EXIT_FAILURE;
  } else {
    return EXIT_SUCCESS;
  }
}

bool run(test_entry test) {
  clock_t before = clock();
  test_result ret = test.func();
  clock_t total = clock() - before;

  double time = total / (CLOCKS_PER_SEC * 1.0);

  if(ret.ok) {
    printf("%-30s \033[0;32mpassed\033[0m in %4.3lfs.\n", test.name, time);
  } else {
    printf("%-30s \033[0;31mfailed\033[0m in %4.3lfs.\n", test.name, time);
    printf(
        "    \033[0;31m%s\033[0m failed at %s:%zi\n",
        ret.assertion,
        ret.func,
        ret.line);
  }

  return ret.ok;
}
