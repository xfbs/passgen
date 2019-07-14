#include "tests/tests.h"
#include <time.h>

test_ret test_ok = {.ok = true};

int main(int argc, char *argv[]) {
  printf("\033[1;34m=>\033[0m running %s.\n", argv[0]);

  size_t failures = 0;
  size_t success = 0;
  for (size_t i = 0; tests[i].name; ++i) {
    if (run(tests[i])) {
      success += 1;
    } else {
      failures += 1;
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
