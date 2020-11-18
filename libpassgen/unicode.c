#include "passgen/unicode.h"
#include <string.h>
#include <utf8proc.h>

#define MIN_BUFLEN 8

struct unicode_iter unicode_iter(const char *data) {
     return unicode_iter_sized(data, strlen(data));
}

struct unicode_iter unicode_iter_sized(const char *data, size_t length) {
     struct unicode_iter iter = {
         .data = data,
         .length = length,
         .pos = 0,
     };

     return iter;
}

struct unicode_iter_result unicode_iter_peek(const struct unicode_iter *iter) {
     struct unicode_iter_result result;

     if(iter->pos >= iter->length) {
          result.ok = false;
          result.error = 0;
          result.codepoint = 0;
          return result;
     }

     result.error = utf8proc_iterate(
         (const utf8proc_uint8_t *) (iter->data) + iter->pos,
         iter->length - iter->pos,
         &result.codepoint);

     if(result.error > 0) {
          result.ok = true;
     } else {
          result.ok = false;
     }

     return result;
}

struct unicode_iter_result unicode_iter_next(struct unicode_iter *iter) {
     struct unicode_iter_result result = unicode_iter_peek(iter);

     if(result.ok) {
          iter->pos += result.error;
     }

     return result;
}
