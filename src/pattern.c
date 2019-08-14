#include "passgen/pattern.h"
#include <assert.h>
#include <stdbool.h>

bool is_illegal(char c) {
  return c == ']' || c == ')';
}
bool is_end(char c) {
  return c == '\0';
}
bool is_sep(char c) {
  return c == '|';
}

typedef uint32_t parse_char_func(const char **string);

uint32_t parse_char_unicode(const char **string);

// list of escape chars and what they map to. has either a char it maps to or
// a function that is called to parse it further.
static struct escaped_chars {
  // when encountering a sequence like "\a", it checks for an entry where chr
  // is 'a'.
  uint32_t chr;
  // the char the sequence "\a" maps to, in this case the character '\a'
  // (alert).
  uint32_t dest;
  // If dest is zero, we instead call this function to parse whatever is coming
  // after, to be able to parse things like "\u{ABCD}".
  parse_char_func *func;
} escape_chars[] = {
  { 'a', '\a', NULL },
  { 'b', '\b', NULL },
  { 'e', '\033', NULL },
  { 'f', '\f', NULL },
  { 'n', '\n', NULL },
  { 'r', '\r', NULL },
  { 't', '\t', NULL },
  { '\\', '\\', NULL },
  //{'u', 0, parse_char_unicode},
  { 0, 0, NULL },
};

pattern_range_t *pattern_range_new(
    char start,
    char end,
    pattern_range_t *next) {
  pattern_range_t *new = malloc(sizeof(pattern_range_t));
  assert(new);
  new->start = start;
  new->end = end;
  new->next = next;
  return new;
}

// parses and consumes a character (incrementing string).
// returns 0 on error.
uint32_t parse_char(const char **string) {
  if(**string == '\0') {
    return 0;
  }

  // this character might be escaped.
  if(**string == '\\') {
    for(size_t i = 0; escape_chars[i].chr; ++i) {
      if((*string)[1] == escape_chars[i].chr) {
        *string += 2;

        if(escape_chars[i].dest) {
          return escape_chars[i].dest;
        } else {
          return escape_chars[i].func(string);
        }
      }
    }

    // nothing matched.
    return 0;
  }

  uint32_t out = **string;

  *string += 1;
  return out;
}

pattern_range_t *pattern_range_parse(const char **string) {
  if(is_illegal(**string) || is_end(**string) || is_sep(**string)) return NULL;

  char start = parse_char(string);

  if(!start) {
    return NULL;
  }

  char end;
  if(**string == '-') {
    // skip the dash.
    *string += 1;
    end = parse_char(string);

    // make sure end exists.
    if(!end) {
      return NULL;
    }

    // make sure end is smaller than start.
    if(start > end) {
      return NULL;
    }
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
  if(!range) return;

  if(range->next) {
    pattern_range_free(range->next);
  }

  free(range);
}

char pattern_range_random(pattern_range_t *range, random_t *rand) {
  // calculate sum of possible chars
  size_t poss = 0;
  for(pattern_range_t *pos = range; pos != NULL; pos = pos->next) {
    poss += pos->end - pos->start + 1;
  }

  size_t choice = random_uint64_max(rand, poss);

  for(pattern_range_t *pos = range; pos != NULL; pos = pos->next) {
    size_t count = pos->end - pos->start + 1;

    if(choice < count) {
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
    pattern_reps_t reps = { .min = 0, .max = 0 };
    return pattern_segment_new(PATTERN_CHAR, (void *)*string, reps, NULL);
  }

  // parse the kind and data.
  pattern_kind kind;
  void *data = NULL;
  switch(**string) {
    case '[':
      *string += 1;
      kind = PATTERN_RANGE;
      data = pattern_range_parse(string);
      if(!data || **string != ']') goto fail;
      break;
    case '(':
      *string += 1;
      kind = PATTERN_GROUP;
      data = pattern_parse(string);
      if(!data || **string != ')') goto fail;
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
  pattern_reps_t reps = { .min = 1, .max = 1 };
  if(**string == '{') {
    *string += 1;
    reps = pattern_segment_parse_reps(string);
    if(**string != '}') goto fail;
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

pattern_segment_t *pattern_segment_new(
    pattern_kind kind,
    void *data,
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
  if(!pattern) return;

  switch(pattern->kind) {
    case PATTERN_RANGE:
      pattern_range_free(pattern->data.range);
      break;
    case PATTERN_GROUP:
      pattern_free(pattern->data.group);
    case PATTERN_CHAR:
      break;
  }

  if(pattern->next) {
    pattern_segment_free(pattern->next);
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

pattern_reps_t pattern_segment_parse_reps(const char **string) {
  pattern_reps_t reps = { .min = 0, .max = 0 };

  reps.min = parse_size(string);

  // TODO: error handling. what if {,}?
  if(**string == ',') {
    *string += 1;
    reps.max = parse_size(string);
  } else {
    reps.max = reps.min;
  }

  return reps;
}

size_t pattern_segment_maxlen(pattern_segment_t *pattern) {
  size_t len = 0;

  switch(pattern->kind) {
    case PATTERN_RANGE:
    case PATTERN_CHAR:
      len += pattern->reps.max;
      break;
    case PATTERN_GROUP:
      // TODO
      break;
  }

  if(!pattern->next) {
    return len;
  } else {
    return len + pattern_segment_maxlen(pattern->next);
  }
}

size_t pattern_segment_random_fill(
    pattern_segment_t *segment,
    random_t *rand,
    char *str,
    size_t len) {
  size_t written = 0;

  while(written < len && segment) {
    // determine how often this should be repeated.
    size_t reps = segment->reps.min;
    if(segment->reps.min != segment->reps.max) {
      reps += random_uint64_max(rand, segment->reps.max - segment->reps.min);
    }

    // repeat segment reps times.
    for(size_t i = 0; i < reps && written < len; ++i) {
      switch(segment->kind) {
        case PATTERN_CHAR:
          str[written] = segment->data.chr[0];
          written += 1;
          break;
        case PATTERN_RANGE:
          str[written] = pattern_range_random(segment->data.range, rand);
          written += 1;
          break;
        case PATTERN_GROUP:
          written += pattern_random_fill(
              segment->data.group, rand, &str[written], len - written);
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
  if(!pattern) return;

  pattern_t *next;

  while(pattern) {
    pattern_segment_free(pattern->item);
    next = pattern->next;
    free(pattern);
    pattern = next;
  }
}

pattern_t *pattern_parse(const char **string) {
  pattern_t *pattern = pattern_new(pattern_segment_parse(string), NULL);

  pattern_t *rest = pattern;
  while(!is_illegal(**string) && !is_end(**string)) {
    *string += 1;
    rest->next = pattern_new(pattern_segment_parse(string), NULL);
    rest = rest->next;
  }

  return pattern;
}

size_t pattern_maxlen(pattern_t *pattern) {
  size_t maxlen = 0;

  // return the largest segment's maxlen.
  while(pattern) {
    size_t maxlen_cur = pattern_segment_maxlen(pattern->item);
    if(maxlen_cur > maxlen) maxlen = maxlen_cur;
  }

  return maxlen;
}

size_t pattern_segment_count(pattern_t *pattern) {
  size_t count = 0;

  while(pattern) {
    count += 1;
    pattern = pattern->next;
  }

  return count;
}

size_t pattern_random_fill(
    pattern_t *pattern,
    random_t *rand,
    char *buffer,
    size_t len) {
  // choose a segment
  size_t segments = pattern_segment_count(pattern);
  size_t choice = random_uint64_max(rand, segments);

  // find segment
  while(choice) {
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
  while(choice) {
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

pattern_error_t pattern_error(
    enum pattern_error_kind kind,
    size_t prev,
    size_t pos) {
  return (pattern_error_t){
    .kind = kind,
    .prev = prev,
    .pos = pos,
  };
}

const char *pattern_error_fmtstr(pattern_error_t error) {
  switch(error.kind) {
    case PATTERN_ERROR_UNKNOWN:
      return "Unknown error happened while parsing.";
    case PATTERN_ERROR_ALLOC:
      return "Error happened while allocating memory.";
    default:
      return "Illegal error happened.";
  }
}

struct pattern_token pattern_token_error(size_t pos) {
  return (struct pattern_token) {
    .type = PATTERN_TOKEN_ERROR,
    .data.offset = pos
  };
}

struct pattern_token pattern_token_eof(void) {
  return (struct pattern_token) {
    .type = PATTERN_TOKEN_EOF,
  };
}

struct pattern_token pattern_token_regular(int32_t codepoint) {
  return (struct pattern_token) {
    .type = PATTERN_TOKEN_REGULAR,
    .codepoint = codepoint
  };
}

struct pattern_token pattern_token_escaped(int32_t codepoint) {
  return (struct pattern_token) {
    .type = PATTERN_TOKEN_ESCAPED,
    .codepoint = codepoint
  };
}

/// Parse the next token, without advancing the unicode reader.
struct pattern_token pattern_token_peek(const struct unicode_iter *iter) {
  struct unicode_iter iter_copy = *iter;
  return pattern_token_next(&iter_copy);
}

struct pattern_token pattern_token_unicode(struct unicode_iter *iter) {
}

struct pattern_token pattern_token_wordlist(struct unicode_iter *iter) {
}

typedef struct pattern_token pattern_token_special(struct unicode_iter *iter);

struct special_chars {
  int32_t chr;
  int32_t dest;
  pattern_token_special *parser;
};

static struct special_chars special_chars[] = {
  {'a', '\a', NULL}, /// alert
  {'b', '\b', NULL}, /// no idea
  {'e', '\033', NULL}, /// escape
  {'f', '\f', NULL}, /// no idea
  {'n', '\n', NULL}, /// newline
  {'r', '\r', NULL}, /// carriage return
  {'t', '\t', NULL}, /// tab
  {'v', '\v', NULL}, /// vertical tab
  {'\\', '\\', NULL}, /// backspace
  {'u', 0, pattern_token_unicode},
  {'w', 0, pattern_token_wordlist},
};

static const size_t special_chars_size = sizeof(special_chars);

struct pattern_token pattern_token_next_complex(struct unicode_iter *iter) {
  struct unicode_iter_result result;

  // this is not a regular token. see what comes next.
  result = unicode_iter_next(iter);
  if(!result.ok) {
    return pattern_token_error(iter->pos);
  }

  for(size_t i = 0; i < special_chars_size; ++i) {
    if(result.codepoint == special_chars[i].chr) {
      if(special_chars[i].parser) {
        return special_chars[i].parser(iter);
      } else {
        return pattern_token_escaped(special_chars[i].dest);
      }
    }
  }

  return pattern_token_error(iter->pos);
}

/// Parse the next token, advancing the unicode reader.
struct pattern_token pattern_token_next(struct unicode_iter *iter) {
  struct unicode_iter_result result;

  result = unicode_iter_next(iter);
  if(!result.ok) {
    if(result.error == 0 && result.codepoint == 0) {
      return pattern_token_eof();
    } else {
      return pattern_token_error(iter->pos);
    }
  }

  // this is a regular token.
  if(result.codepoint != '\\') {
    return pattern_token_regular(result.codepoint);
  }

  return pattern_token_next_complex(iter);
}
