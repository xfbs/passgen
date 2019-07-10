#include "passgen/reader.h"
#include <string.h>

read_ret reader_fread(reader_t *reader, void *dest, size_t size) {
  size_t read = fread(dest, 1, size, reader->data);

  read_ret ret = {
    .ok = true,
    .eof = feof(reader->data),
    .error = false,
    .errno = 0,
    .size = read,
  };

  return ret;
}

read_ret reader_string_read(reader_t *reader, void *dest, size_t size) {
  char *pos = stpncpy((char *) dest, (const char *) reader->data, size);
  size_t read = pos - (char *) dest;

  // advance pointer.
  reader->data = ((char *) reader->data) + read;

  read_ret ret = {
    .ok = true,
    .eof = read < size,
    .error = false,
    .errno = 0,
    .size = size
  };

  return ret;
}

reader_t reader_from_file(FILE *file) {
  reader_t reader = {
    .read = reader_fread,
    .data = file,
  };

  return reader;
}

reader_t reader_from_string(const char *string);

read_ret reader_read(reader_t *r, void *dest, size_t size, size_t amount);

bufreader_t bufreader_from_reader(reader_t reader);

read_ret bufreader_read(bufreader_t *reader, void *dest, size_t size, size_t amount);
