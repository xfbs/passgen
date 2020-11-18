#include "passgen/reader.h"

#define STATUS_EOF        1
#define is_status_eof(n)  ((n) &STATUS_EOF)
#define set_status_eof(n) (n) |= STATUS_EOF

read_result reader_fread(reader_t *reader, void *dest, size_t size) {
     size_t read = fread(dest, 1, size, reader->data);

     reader->pos += read;

     read_result ret = {
         .ok = !ferror(reader->data),
         .eof = feof(reader->data),
         .errno = 0,
         .read = read,
     };

     return ret;
}

read_result reader_string_read(reader_t *reader, void *_dest, size_t size) {
     const char *data = reader->data;
     char *dest = _dest;
     size_t pos = reader->pos;
     size_t read;
     bool eof = false;

     // read until we hit a NULL character.
     for(read = 0; read < size; ++read) {
          if(!data[pos + read]) {
               eof = true;
               break;
          }

          dest[read] = data[pos + read];
     }

     // update the position in the string.
     reader->pos += read;

     // still write out termination if hit NULL.
     if(eof && !is_status_eof(reader->status)) {
          dest[read] = '\0';
          read += 1;
          set_status_eof(reader->status);
     }

     read_result ret = {
         .ok = true,
         .eof = eof,
         .errno = 0,
         .read = read,
     };

     return ret;
}

reader_t reader_from_file(FILE *file) {
     reader_t reader = {
         .read = reader_fread,
         .data = file,
         .pos = 0,
     };

     return reader;
}

reader_t reader_from_string(const char *string) {
     reader_t reader = {
         .read = reader_string_read,
         .data = (void *) string,
         .pos = 0,
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
