#include "passgen/reader.h"
#include <string.h>

read_result reader_fread(reader_t *reader, void *dest, size_t size) {
  size_t read = fread(dest, 1, size, reader->data);

  reader->pos += read;

  read_result ret = {
    .ok = !ferror(reader->data),
    .eof = feof(reader->data),
    .errno = 0,
    .size = read,
  };

  return ret;
}

read_result reader_string_read(reader_t *reader, void *dest, size_t size) {
  char *pos = stpncpy((char *) dest, (const char *) reader->data, size);
  size_t read = pos - (char *) dest;

  // advance pointer.
  reader->data = ((char *) reader->data) + read;
  reader->pos += read;

  read_result ret = {
    .ok = true,
    .eof = read < size,
    .errno = 0,
    .size = size
  };

  return ret;
}

reader_t reader_from_file(FILE *file) {
  reader_t reader = {
    .read = reader_fread,
    .data = file,
    .pos  = 0,
  };

  return reader;
}

reader_t reader_from_string(const char *string) {
  reader_t reader = {
    .read = reader_string_read,
    .data = (void *) string,
    .pos  = 0,
  };

  return reader;
}

read_result reader_read(reader_t *reader, void *dest, size_t size) {
  return reader->read(reader, dest, size);
}

size_t reader_pos(reader_t *reader) {
  return reader->pos;
}

reader_t bufreader_from_reader(reader_t reader);

read_result bufreader_read(bufreader_t *reader, void *dest, size_t size);
