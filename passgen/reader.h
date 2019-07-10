#pragma once
#include <stdio.h>
#include <stdbool.h>

struct read_ret;
struct reader_t;
struct bufreader_t;

struct read_ret {
  size_t size;
  bool ok;
  bool eof;
  bool error;
  int errno;
};

typedef struct read_ret read_function(struct reader_t *reader, void *dest, size_t size);

struct reader_t {
  read_function *read;
  void *data;
};

struct bufreader_t {
  struct reader_t reader;
  char buffer[256];
};

typedef struct reader_t reader_t;
typedef struct bufreader_r bufreader_t;
typedef struct read_ret read_ret;

reader_t reader_from_file(FILE *file);
reader_t reader_from_string(const char *string);

read_ret reader_read(reader_t *r, void *dest, size_t size, size_t amount);

bufreader_t bufreader_from_reader(reader_t reader);

read_ret bufreader_read(bufreader_t *reader, void *dest, size_t size, size_t amount);
