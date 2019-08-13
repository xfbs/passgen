/// @file unicode.h
///
/// Methods for decoding unicode characters from a reader.

#pragma once
#include <stdint.h>
#include "passgen/reader.h"

struct unicode_iter {
  const char *data;
  size_t length;
  size_t pos;
};

struct unicode_iter_result {
  bool ok;
  int32_t codepoint;
  ssize_t error;
};

/// Creates a new unicode iterator.
struct unicode_iter unicode_iter(const char *data);

/// Creates a new unicode iterator from a given reader.
struct unicode_iter unicode_iter_sized(const char *data, size_t length);

/// Get the next unicode character without advancing the iterator.
struct unicode_iter_result unicode_iter_peek(const struct unicode_iter *iter);

/// Read the next unicode character.
struct unicode_iter_result unicode_iter_next(struct unicode_iter *iter);
