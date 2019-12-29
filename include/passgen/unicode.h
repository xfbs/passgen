/// @file unicode.h
///
/// Methods for decoding unicode characters from a reader.

#pragma once
#include <stdint.h>
#include <sys/types.h>
#include "passgen/reader.h"

/// Unicode iterator. Iterates through a string and returns unicode
/// characters on-demand.
struct unicode_iter {
  /// The string to iterate over. Neither allocated nor freed by us.
  const char *data;

  /// How many bytes the string has.
  size_t length;

  /// Offset (in bytes) into the string.
  size_t pos;
};

/// Result of decoding a character.
struct unicode_iter_result {
  /// Whether the operation succeeded.
  bool ok;

  /// The decoded codepoint.
  int32_t codepoint;

  /// The error code (if an error happened).
  ssize_t error;
};

typedef struct unicode_iter unicode_iter_t;
typedef struct unicode_iter_result unicode_iter_result_t;

/// Creates a new unicode iterator.
struct unicode_iter unicode_iter(const char *data);

/// Creates a new unicode iterator from a given reader.
struct unicode_iter unicode_iter_sized(const char *data, size_t length);

/// Get the next unicode character without advancing the iterator.
struct unicode_iter_result unicode_iter_peek(const struct unicode_iter *iter);

/// Read the next unicode character.
struct unicode_iter_result unicode_iter_next(struct unicode_iter *iter);
