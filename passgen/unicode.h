/// @file unicode.h
///
/// Methods for decoding unicode characters from a reader.

#pragma once
#include <stdint.h>
#include "passgen/reader.h"

struct unicode_reader_t;

struct unicode_reader_t {
  /// The reader to read from.
  reader_t reader;

  /// The buffer, since we don't know how long each unicode character is.
  char buffer[4];

  /// How much data is still in the buffer.
  size_t buffered;

  /// How many unicode characters we've read.
  size_t amount;
};

typedef struct unicode_reader_t unicode_reader_t;

/// Creates a new unicode reader from a given reader.
unicode_reader_t unicode_reader(reader_t reader);

/// Read @p amount unicode characters into @p dest.
read_result unicode_read(
    unicode_reader_t *reader,
    uint32_t *dest,
    size_t amount);

/// How many unicode characters we've read so far.
size_t unicode_amount(unicode_reader_t *reader);

/// How many bytes we've consumed so far.
size_t unicode_size(unicode_reader_t *reader);
