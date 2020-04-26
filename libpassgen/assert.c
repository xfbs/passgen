#include "passgen/assert.h"

#include <stdlib.h>

#ifdef PASSGEN_DEBUG

#  ifdef PASSGEN_BACKTRACE
#    include <execinfo.h>
#  endif

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
      "Assertion failed in file %s at %s:%zu\n  \033[31m%s\033[0m\n",
      file,
      func,
      line,
      cond);

#  ifdef PASSGEN_BACKTRACE
  // print backtrace
  fprintf(stderr, "\nBacktrace:\n");

  void *callstack[CALLSTACK_MAX];
  int frames = backtrace(callstack, sizeof(callstack));
  char **strs = backtrace_symbols(callstack, frames);

  for(int i = 0; i < frames; ++i) {
    printf("%s\n", strs[i]);
  }

  free(strs);
#  endif

  exit(EXIT_FAILURE);
}
#endif
