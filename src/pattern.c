#include "passgen/pattern.h"
#include <assert.h>
#include <stdbool.h>

bool is_illegal(char c) { return c == ']' || c == ')'; }
bool is_end(char c) { return c == '\0'; }
bool is_sep(char c) { return c == '|'; }

pattern_range_t *pattern_range_new(char start, char end,
                                   pattern_range_t *next) {
  pattern_range_t *new = malloc(sizeof(pattern_range_t));
  assert(new);
  new->start = start;
  new->end = end;
  new->next = next;
  return new;
}

pattern_range_t *pattern_range_parse(const char **string) {
  if (is_illegal(**string) || is_end(**string) || is_sep(**string)) return NULL;

  // char might be escaped.
  if(**string == '\\') {
    *string += 1;
  }

  char start = **string;
  *string += 1;

  if(!start) {
    return NULL;
  }

  char end;
  if (**string == '-') {
    // is a range.
    *string += 1;
    end = **string;
    *string += 1;
  } else {
    // is a single char.
    end = start;
  }

  if(!end) {
    return NULL;
  }

  return pattern_range_new(start, end, pattern_range_parse(string));
}

void pattern_range_free(pattern_range_t *range) {
  if (!range) return;

  if (range->next) {
    pattern_range_free(range->next);
  }

  free(range);
}

char pattern_range_random(pattern_range_t *range, random_t *rand) {
  // calculate sum of possible chars
  size_t poss = 0;
  for (pattern_range_t *pos = range; pos != NULL; pos = pos->next) {
    poss += pos->end - pos->start + 1;
  }

  size_t choice = random_uint64_max(rand, poss);

  for (pattern_range_t *pos = range; pos != NULL; pos = pos->next) {
    size_t count = pos->end - pos->start + 1;

    if (choice < count) {
      return pos->start + choice;
    } else {
      choice -= count;
    }
  }

  // unreachable.
  return '\0';
}

pattern_segment_t *pattern_segment_parse(const char **string) {
  if(is_end(**string) || is_sep(**string) || is_illegal(**string)) {
    // TODO: empty segment.
    pattern_reps_t reps = {.min = 0, .max = 0};
    return pattern_segment_new(PATTERN_CHAR, (void *) *string, reps, NULL);
  }

  // parse the kind and data.
  pattern_kind kind;
  void *data = NULL;
  switch (**string) {
    case '[':
      *string += 1;
      kind = PATTERN_RANGE;
      data = pattern_range_parse(string);
      if (!data || **string != ']') goto fail;
      break;
    case '(':
      *string += 1;
      kind = PATTERN_GROUP;
      data = pattern_parse(string);
      if (!data || **string != ')') goto fail;
      break;
    case '\\':
      *string += 1;
      kind = PATTERN_CHAR;
      data = (void *)*string;
    default:
      kind = PATTERN_CHAR;
      data = (void *)*string;
  }

  *string += 1;

  // parse the repetitions, if any.
  pattern_reps_t reps = {.min = 1, .max = 1};
  if (**string == '{') {
    *string += 1;
    reps = pattern_segment_parse_reps(string);
    if (**string != '}') goto fail;
    *string += 1;
  }

  // don't recurse if we're at the end.
  if(is_end(**string) || is_sep(**string) || is_illegal(**string)) {
    return pattern_segment_new(kind, data, reps, NULL);
  }

  return pattern_segment_new(kind, data, reps, pattern_segment_parse(string));
fail:
  if(data) {
    switch(kind) {
      case PATTERN_GROUP:
        pattern_free(data);
        break;
      case PATTERN_RANGE:
        pattern_range_free(data);
        break;
      case PATTERN_CHAR:
        break;
    }
  }

  return NULL;
}

pattern_segment_t *pattern_segment_new(pattern_kind kind, void *data,
                                       pattern_reps_t reps,
                                       pattern_segment_t *next) {
  pattern_segment_t *pattern = malloc(sizeof(pattern_segment_t));
  assert(pattern);

  pattern->kind = kind;
  pattern->data.raw = data;
  pattern->reps = reps;
  pattern->next = next;

  return pattern;
}

void pattern_segment_free(pattern_segment_t *pattern) {
  if (!pattern) return;

  switch (pattern->kind) {
    case PATTERN_RANGE:
      pattern_range_free(pattern->data.range);
      break;
    case PATTERN_GROUP:
      pattern_free(pattern->data.group);
    case PATTERN_CHAR:
      break;
  }

  if (pattern->next) {
    pattern_segment_free(pattern->next);
  }

  free(pattern);
}

size_t parse_size(const char **string) {
  size_t out = 0;

  while ('0' <= **string && **string <= '9') {
    out *= 10;
    out += (**string - '0');
    *string += 1;
  }

  return out;
}

pattern_reps_t pattern_segment_parse_reps(const char **string) {
  pattern_reps_t reps = {.min = 0, .max = 0};

  reps.min = parse_size(string);

  // TODO: error handling. what if {,}?
  if (**string == ',') {
    *string += 1;
    reps.max = parse_size(string);
  } else {
    reps.max = reps.min;
  }

  return reps;
}

size_t pattern_segment_maxlen(pattern_segment_t *pattern) {
  size_t len = 0;

  switch (pattern->kind) {
    case PATTERN_RANGE:
    case PATTERN_CHAR:
      len += pattern->reps.max;
      break;
    case PATTERN_GROUP:
      // TODO
      break;
  }

  if (!pattern->next) {
    return len;
  } else {
    return len + pattern_segment_maxlen(pattern->next);
  }
}

size_t pattern_segment_random_fill(pattern_segment_t *segment, random_t *rand,
                                   char *str, size_t len) {
  size_t written = 0;

  while (written < len && segment) {
    // determine how often this should be repeated.
    size_t reps = segment->reps.min;
    if (segment->reps.min != segment->reps.max) {
      reps += random_uint64_max(rand, segment->reps.max - segment->reps.min);
    }

    // repeat segment reps times.
    for (size_t i = 0; i < reps && written < len; ++i) {
      switch (segment->kind) {
        case PATTERN_CHAR:
          str[written] = segment->data.chr[0];
          written += 1;
          break;
        case PATTERN_RANGE:
          str[written] = pattern_range_random(segment->data.range, rand);
          written += 1;
          break;
        case PATTERN_GROUP:
          written += pattern_random_fill(segment->data.group, rand,
                                         &str[written], len - written);
          break;
      }
    }

    segment = segment->next;
  }

  return written;
}

char *pattern_segment_random(pattern_segment_t *pattern, random_t *rand) {
  // create buffer that is big enough.
  size_t len = pattern_segment_maxlen(pattern);
  char *buffer = malloc(len + 1);
  assert(buffer);

  // write.
  size_t written = pattern_segment_random_fill(pattern, rand, buffer, len);

  // null-terminate.
  buffer[written] = '\0';

  return buffer;
}

pattern_t *pattern_new(pattern_segment_t *segment, pattern_t *next) {
  pattern_t *pattern = malloc(sizeof(pattern_t));
  assert(pattern);
  pattern->item = segment;
  pattern->next = next;
  return pattern;
}

void pattern_free(pattern_t *pattern) {
  if (!pattern) return;

  pattern_t *next;

  while (pattern) {
    pattern_segment_free(pattern->item);
    next = pattern->next;
    free(pattern);
    pattern = next;
  }
}

pattern_t *pattern_parse(const char **string) {
  pattern_t *pattern = pattern_new(pattern_segment_parse(string), NULL);

  pattern_t *rest = pattern;
  while (!is_illegal(**string) && !is_end(**string)) {
    *string += 1;
    rest->next = pattern_new(pattern_segment_parse(string), NULL);
    rest = rest->next;
  }

  return pattern;
}

size_t pattern_maxlen(pattern_t *pattern) {
  size_t maxlen = 0;

  // return the largest segment's maxlen.
  while (pattern) {
    size_t maxlen_cur = pattern_segment_maxlen(pattern->item);
    if (maxlen_cur > maxlen) maxlen = maxlen_cur;
  }

  return maxlen;
}

size_t pattern_segment_count(pattern_t *pattern) {
  size_t count = 0;

  while (pattern) {
    count += 1;
    pattern = pattern->next;
  }

  return count;
}

size_t pattern_random_fill(pattern_t *pattern, random_t *rand, char *buffer,
                           size_t len) {
  // choose a segment
  size_t segments = pattern_segment_count(pattern);
  size_t choice = random_uint64_max(rand, segments);

  // find segment
  while (choice) {
    pattern = pattern->next;
    choice -= 1;
  }

  return pattern_segment_random_fill(pattern->item, rand, buffer, len);
}

char *pattern_random(pattern_t *pattern, random_t *rand) {
  // choose a segment
  size_t segments = pattern_segment_count(pattern);
  size_t choice = random_uint64_max(rand, segments);

  // find segment
  while (choice) {
    pattern = pattern->next;
    choice -= 1;
  }

  return pattern_segment_random(pattern->item, rand);
}

size_t pattern_choices(pattern_t *pattern) {
  size_t choices = 0;

  while(pattern) {
    choices += pattern_segment_choices(pattern->item);
    pattern = pattern->next;
  }

  return choices;
}

size_t pattern_range_choices(pattern_range_t *range) {
  size_t choices = 0;

  while(range) {
    choices += range->end - range->start + 1;
    range = range->next;
  }

  return choices;
}

size_t pattern_segment_choices(pattern_segment_t *segment) {
  size_t choices = 0;

  while(segment) {
    size_t lchoices;
    switch(segment->kind) {
      case PATTERN_CHAR:
        lchoices = 1;
        break;
      case PATTERN_RANGE:
        lchoices = pattern_range_choices(segment->data.range);
        break;
      case PATTERN_GROUP:
        lchoices = pattern_choices(segment->data.group);
        break;
    }

    size_t mchoices = 1;
    for(size_t i = 0; i < segment->reps.min; ++i) {
      mchoices *= lchoices;
    }

    for(size_t num = segment->reps.min; num <= segment->reps.max; ++num) {
      choices += mchoices;
      mchoices *= lchoices;
    }

    segment = segment->next;
  }

  return choices;
}
