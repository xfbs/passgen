#include "passgen/assert.h"

#include <stdlib.h>

#ifdef PASSGEN_DEBUG
#  include <execinfo.h>
#  include <stdio.h>

#  define CALLSTACK_MAX 128

void passgen_assert_fail(
    const char *cond,
    const char *file,
    const char *func,
    size_t line) {
  // print warning
  fprintf(
      stderr,
      "Assertion failed: in file %s, func %s:%zu\n    %s\n",
      file,
      func,
      line,
      cond);

  // print backtrace
  fprintf(stderr, "\nBacktrace:\n");
  void *callstack[CALLSTACK_MAX];
  int frames = backtrace(callstack, sizeof(callstack));
  char **strs = backtrace_symbols(callstack, frames);
  for(int i = 0; i < frames; ++i) {
    printf("%s\n", strs[i]);
  }
  free(strs);

  exit(EXIT_FAILURE);
}
#endif
