#include "passgen/reader.h"
#include <stdbool.h>
#include <stdio.h>
#include "tests/tests.h"

test_result test_reader_string() {
  reader_t reader = reader_from_string("abcde");
  char data[100] = { 0 };

  assert(reader_pos(&reader) == 0);

  read_result res;

  res = reader_read(&reader, &data[0], 1);
  assert(reader_pos(&reader) == 1);
  assert(res.ok);
  assert(res.eof == false);
  assert(res.read == 1);
  assert(data[0] == 'a');
  assert(data[1] == 0);

  res = reader_read(&reader, &data[0], 1);
  assert(reader_pos(&reader) == 2);
  assert(res.ok);
  assert(res.eof == false);
  assert(res.read == 1);
  assert(data[0] == 'b');
  assert(data[1] == 0);

  res = reader_read(&reader, &data[0], 2);
  assert(reader_pos(&reader) == 4);
  assert(res.ok);
  assert(res.eof == false);
  assert(res.read == 2);
  assert(data[0] == 'c');
  assert(data[1] == 'd');
  assert(data[2] == 0);

  // reading past EOF should only read up to EOF.
  res = reader_read(&reader, &data[0], 10);
  assert(reader_pos(&reader) == 5);
  assert(res.ok);
  assert(res.eof == true);
  assert(res.read == 2);
  assert(data[0] == 'e');
  assert(data[1] == '\0');

  // once EOF has been set, don't read any more data.
  res = reader_read(&reader, &data[0], 10);
  assert(reader_pos(&reader) == 5);
  assert(res.ok);
  assert(res.eof == true);
  assert(res.read == 0);

  return test_ok;
}

// clang-format off
test_entry tests[] = {
  test(reader_string),
  {NULL, NULL}
};
// clang-format on
