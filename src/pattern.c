#include <assert.h>
#include <stdbool.h>
#include "pattern.h"

bool is_end(char c) {
  return c == ']' || c == ')' || c == '\0';
}

pattern_range_t *pattern_range_new(char start, char end, pattern_range_t *next) {
  pattern_range_t *new = malloc(sizeof(pattern_range_t));
  assert(new);
  new->start = start;
  new->end = end;
  new->next = next;
  return new;
}

pattern_range_t *pattern_range_parse(const char **string) {
  if(is_end(**string)) return NULL;

  // TODO: read escaped \] \).
  char start = (*string)[0];
  char end;

  if((*string)[1] == '-') {
    // is a range.
    if((*string)[2] == '\0') return NULL;
    end = (*string)[2];
    *string += 3;
  } else {
    // is a single char.
    end = start;
    *string += 1;
  }

  return pattern_range_new(start, end, pattern_range_parse(string));
}

void pattern_range_free(pattern_range_t *range) {
  if(range->next) pattern_range_free(range->next);
  free(range);
}

pattern_t *pattern_parse(const char **string) {
  if(is_end(**string)) return NULL;

  // parse the kind and data.
  pattern_kind kind;
  void *data;
  switch(**string) {
    case '[':
      *string += 1;
      kind = PATTERN_RANGE;
      data = pattern_range_parse(string);
      if(**string != ']') goto fail;
      break;
    case '(':
      *string += 1;
      kind = PATTERN_GROUP;
      data = NULL;
      if(**string != ')') goto fail;
      break;
    case '\\':
      *string += 1;
      kind = PATTERN_CHAR;
      data = (void *) *string;
    default:
      kind = PATTERN_CHAR;
      data = (void *) *string;
  }

  *string += 1;

  // parse the repetitions, if any.
  pattern_reps_t repetitions = {.min = 1, .max = 1};
  if(**string == '{') {
    *string += 1;
    repetitions = pattern_parse_reps(string);
    if(**string != '}') goto fail;
    *string += 1;
  }

  return NULL;

fail:
  return NULL;
}

void pattern_free(pattern_t *pattern) {
  switch(pattern->kind) {
    case PATTERN_RANGE: pattern_range_free(pattern->data); break;
    case PATTERN_GROUP: break;
    case PATTERN_CHAR:  break;
  }

  if(pattern->next) {
    pattern_free(pattern->next);
  }

  free(pattern);
}

size_t parse_size(const char **string) {
  size_t out = 0;

  while('0' <= **string && **string <= '9') {
    out *= 10;
    out += (**string - '0');
    *string += 1;
  }

  return out;
}

pattern_reps_t pattern_parse_reps(const char **string) {
  pattern_reps_t reps = {.min = 0, .max = 0};

  reps.min = parse_size(string);

  if(**string == ',') {
    *string += 1;
    reps.max = parse_size(string);
  } else {
    reps.max = reps.min;
  }

  return reps;
}
