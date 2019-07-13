#include "passgen/unicode.h"
#include <utf8proc.h>

#define MIN_BUFLEN 8

unicode_reader_t unicode_reader(reader_t reader) {
  return (unicode_reader_t) {
    .reader = reader,
    .buffer = {0},
    .buffered = 0,
    .amount = 0,
  };
}

read_result unicode_read(unicode_reader_t *reader, uint32_t *dest, size_t amount) {
  size_t read;
  for(read = 0; read < amount; ++read) {
    if(reader->buffered < MIN_BUFLEN) {
    }
  }

  read_result res = {
    .ok = true,
    .eof = true,
    .read = read,
  };

  return res;
}

size_t unicode_amount(unicode_reader_t *reader) {
  return reader->amount;
}

size_t unicode_size(unicode_reader_t *reader) {
  return reader_pos(&reader->reader) - reader->buffered;
}
