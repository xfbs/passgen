#include "tests/tests.h"
#include <time.h>
#include <string.h>

test_ret test_ok = {.ok = true};

int main(int argc, char *argv[]) {
  size_t failures = 0;
  size_t success = 0;
  if(argc > 1) {
    // go through the arguments.
    for(size_t r = 1; r < argc; ++r) {
      // go through the tests.
      for(size_t i = 0; tests[i].name; ++i) {
        // find the test with the name that is the current argument.
        if(0 == strcmp(argv[r], tests[i].name)) {
          // run the testl
          if (run(tests[i])) {
            success += 1;
          } else {
            failures += 1;
          }

          break;
        }
      }
    }
  } else {
    for (size_t i = 0; tests[i].name; ++i) {
      if (run(tests[i])) {
        success += 1;
      } else {
        failures += 1;
      }
    }
  }

  printf("\033[1;34m=>\033[0m %zi/%zi tests passed.\n", success,
         success + failures);

  if (failures) {
    return -1;
  } else {
    return 0;
  }
}

bool run(test_t test) {
  clock_t before = clock();
  test_ret ret = test.fun();
  clock_t total = clock() - before;

  double time = total / (CLOCKS_PER_SEC * 1.0);

  if (ret.ok) {
    printf("%-30s \033[0;32mpassed\033[0m in %4.3lfs.\n", test.name, time);
  } else {
    printf("%-30s \033[0;31mfailed\033[0m in %4.3lfs.\n", test.name, time);
    printf("    \033[0;31m%s\033[0m failed at %s:%zi\n", ret.assertion,
           ret.func, ret.line);
  }

  return ret.ok;
}