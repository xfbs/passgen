#include "passgen/reader.h"
#include "util/tests.h"
#include <stdbool.h>
#include <stdio.h>

test_ret test_reader_string() {
  return test_ok;
}

test_t tests[] = {
  test(reader_string),
  {NULL, NULL}
};
