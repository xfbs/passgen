/// @file reader.h
///
/// Methods for reading from strings and files.

#pragma once
#include <stdio.h>
#include <stdbool.h>

struct read_result;
struct reader_t;
struct bufreader_t;

struct read_result {
  size_t read;
  bool ok;
  bool eof;
  int errno;
};

typedef struct read_result read_function(struct reader_t *reader, void *dest, size_t size);

struct reader_t {
  read_function *read;
  void *data;
  size_t pos;
  size_t len;
  int status;
};

struct bufreader_t {
  struct reader_t reader;
  char buffer[256];
};

typedef struct reader_t reader_t;
typedef struct bufreader_r bufreader_t;
typedef struct read_result read_result;

reader_t reader_from_file(FILE *file);
reader_t reader_from_string(const char *string);

read_result reader_read(reader_t *r, void *dest, size_t size);
size_t reader_pos(reader_t *reader);

reader_t bufreader_from_reader(reader_t reader);
read_result bufreader_read(bufreader_t *reader, void *dest, size_t size);
