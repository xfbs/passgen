#include "passgen/pattern.h"
#include <assert.h>
#include <stdbool.h>

#define accept_regular(chr, error_kind, start) \
    token = pattern_token_peek(iter); \
    if(!passgen_token_regular(&token)) { \
        return pattern_token_error(start, iter->pos, error_kind); \
    }

bool pattern_group_is_separator(passgen_token_t token);
bool pattern_group_is_start(passgen_token_t token);
bool pattern_group_is_end(passgen_token_t token);

bool pattern_ranges_is_start(passgen_token_t token);
bool pattern_ranges_is_end(passgen_token_t token);

bool pattern_segments_is_end(passgen_token_t token);

bool pattern_repeat_is_start(passgen_token_t token);
bool pattern_repeat_is_sep(passgen_token_t token);
bool pattern_repeat_is_end(passgen_token_t token);

pattern_result_t
pattern_group_parse_inner(
        pattern_group_t *group,
        unicode_iter_t *iter,
        size_t depth,
        passgen_mem_t *mem);

pattern_result_t
pattern_group_parse(
        pattern_group_t *group,
        unicode_iter_t *iter,
        size_t depth,
        passgen_mem_t *mem);

pattern_result_t
pattern_ranges_parse_inner(
        pattern_ranges_t *range,
        unicode_iter_t *iter,
        passgen_mem_t *mem);

pattern_result_t
pattern_ranges_parse(
        pattern_ranges_t *range,
        unicode_iter_t *iter,
        passgen_mem_t *mem);

pattern_result_t
pattern_range_parse(
        pattern_range_t *item,
        unicode_iter_t *iter,
        passgen_mem_t *mem);

pattern_result_t
pattern_segments_parse(
        pattern_segments_t *segment,
        unicode_iter_t *iter,
        size_t depth,
        passgen_mem_t *mem);

pattern_result_t
pattern_segment_parse(
        pattern_segment_t *item,
        unicode_iter_t *iter,
        size_t depth,
        passgen_mem_t *mem);

pattern_result_t
pattern_char_parse(
        pattern_char_t *character,
        unicode_iter_t *iter,
        passgen_mem_t *mem);

static inline pattern_result_t
passgen_parse_number(
        size_t *number,
        unicode_iter_t *iter);

static const pattern_result_t result_ok = {
    .ok = true,
};

pattern_result_t pattern_error_illegal(passgen_token_t *token) {
    return (pattern_result_t) {
        .ok = false,
        .kind = PATTERN_ERROR_ILLEGAL,
        .pos.offset = token->pos.offset,
        .pos.length = token->pos.length,
    };
}

pattern_result_t pattern_error_depth(passgen_token_t *token) {
    return (pattern_result_t) {
        .ok = false,
        .remove = true,
        .kind = PATTERN_ERROR_DEPTH,
        .pos.offset = token->pos.offset,
        .pos.length = token->pos.length,
    };
}

pattern_result_t pattern_error_alloc(unicode_iter_t *iter) {
    return (pattern_result_t) {
        .ok = false,
        .kind = PATTERN_ERROR_ILLEGAL,
        .pos.offset = iter->pos,
        .pos.length = 1,
    };
}

static pattern_result_t
pattern_parse_repeat(
        pattern_repeat_t *repeat,
        unicode_iter_t *iter);

void pattern_free(pattern_t *pattern) {
}

pattern_result_t pattern_segments_parse(
        pattern_segments_t *segment,
        unicode_iter_t *iter,
        size_t depth,
        passgen_mem_t *mem)
{
    // save the current position.
    segment->pos.offset = iter->pos;
    segment->pos.length = 0;

    segment->items = passgen_array_init(sizeof(pattern_segment_t), mem);
    bool first = true;

    while(!pattern_segments_is_end(passgen_token_peek(iter))) {
        pattern_segment_t *item = passgen_array_push(
                &segment->items,
                sizeof(pattern_segment_t),
                mem);
        if(!item) {
            return pattern_error_alloc(iter);
        }

        pattern_result_t result = pattern_segment_parse(item, iter, depth, mem);
        if(!result.ok) {
            return result;
        }

        // update length
        segment->pos.length = iter->pos - segment->pos.offset;
        first = false;
    }

    return result_ok;
}

pattern_result_t pattern_group_parse(
        pattern_group_t *group,
        unicode_iter_t *iter,
        size_t depth,
        passgen_mem_t *mem)
{
    return result_ok;
}

pattern_result_t pattern_segment_parse(
        pattern_segment_t *item,
        unicode_iter_t *iter,
        size_t depth,
        passgen_mem_t *mem)
{
    pattern_result_t result;

    passgen_token_t token = passgen_token_peek(iter);

    if(pattern_group_is_start(token)) {
        if(depth == 0) {
            return pattern_error_depth(&token);
        } else {
            item->kind = PATTERN_GROUP;
            result = pattern_group_parse(&item->data.group, iter, depth - 1, mem);
        }
    } else if(pattern_ranges_is_start(token)) {
        item->kind = PATTERN_RANGE;
        result = pattern_ranges_parse(&item->data.range, iter, mem);
    } else {
        item->kind = PATTERN_CHAR;
        result = pattern_char_parse(&item->data.character, iter, mem);
    }

    return result;
}

pattern_result_t pattern_char_parse(
        pattern_char_t *character,
        unicode_iter_t *iter,
        passgen_mem_t *mem)
{
    character->pos.offset = iter->pos;
    passgen_token_t token = passgen_token_next(iter);
    character->pos.length = iter->pos - character->pos.offset;
    character->codepoint = token.codepoint;

    return result_ok;
}

pattern_result_t pattern_ranges_parse_inner(
        pattern_ranges_t *range,
        unicode_iter_t *iter,
        passgen_mem_t *mem)
{
    return result_ok;
}

pattern_result_t pattern_ranges_parse(
        pattern_ranges_t *range,
        unicode_iter_t *iter,
        passgen_mem_t *mem)
{
    return result_ok;
}

/// Parse the content of a group.
pattern_result_t pattern_group_parse_inner(
        pattern_group_t *group,
        unicode_iter_t *iter,
        size_t depth,
        passgen_mem_t *mem)
{
    // save the current position.
    group->pos.offset = iter->pos;
    group->pos.length = 0;

    group->segments = passgen_array_init(sizeof(pattern_segments_t), mem);
    bool first = true;

    while(first || pattern_group_is_separator(passgen_token_peek(iter))) {
        // skip separator.
        if(!first) {
            passgen_token_next(iter);
        }

        pattern_segments_t *segment = passgen_array_push(
                &group->segments,
                sizeof(pattern_segments_t),
                mem);
        if(!segment) {
            return pattern_error_alloc(iter);
        }

        pattern_result_t result = pattern_segments_parse(segment, iter, depth, mem);
        if(!result.ok) {
            return result;
        }

        // update length
        group->pos.length = iter->pos - group->pos.offset;
        first = false;
    }

    return result_ok;
}

pattern_result_t
pattern_parse(
        pattern_t *pattern,
        const char *data,
        size_t depth,
        passgen_mem_t *mem)
{
    pattern->mem = mem;
    pattern->pattern = data;

    // parse segments and make sure everything went okay.
    unicode_iter_t iter = unicode_iter(data);
    pattern_result_t result = pattern_group_parse_inner(&pattern->group, &iter, depth, mem);
    if(!result.ok) {
        return result;
    }

    // force repetitions at one.
    pattern->group.repeat.min = 1;
    pattern->group.repeat.max = 1;

    // make sure we really have reached EOF.
    passgen_token_t token = passgen_token_peek(&iter);
    if(token.type != PATTERN_TOKEN_EOF) {
        return pattern_error_illegal(&token);
    }

    return result_ok;
}

size_t pattern_maxlen(pattern_t *pattern) {
    return 0;
}

size_t pattern_minlen(pattern_t *pattern) {
    return 0;
}

size_t pattern_random_fill(
        pattern_t *pattern,
        random_t *rand,
        char *buffer,
        size_t len)
{
    return 0;
}

size_t pattern_choices(pattern_t *pattern) {
    return 0;
}

bool pattern_group_is_separator(passgen_token_t token) {
    if(token.type == PATTERN_TOKEN_REGULAR && token.codepoint == '|') {
        return true;
    }

    return false;
}

bool pattern_group_is_start(passgen_token_t token) {
    if(token.type == PATTERN_TOKEN_REGULAR && token.codepoint == '(') {
        return true;
    }

    return false;
}

bool pattern_ranges_is_start(passgen_token_t token) {
    if(token.type == PATTERN_TOKEN_REGULAR && token.codepoint == '[') {
        return true;
    }

    return false;
}

bool pattern_ranges_is_end(passgen_token_t token) {
    if(token.type == PATTERN_TOKEN_REGULAR && token.codepoint == ']') {
        return true;
    }

    return false;
}

bool pattern_segments_is_end(passgen_token_t token) {
    if(token.type == PATTERN_TOKEN_REGULAR) {
        switch(token.codepoint) {
            case ']':
            case ')':
            case '|':
                return true;
            default:
                break;
        }
    }

    // FIXME!
    /*
    if(token.type == PATTERN_TOKEN_ERROR) {
        return true;
    }
    */

    if(token.type == PATTERN_TOKEN_EOF) {
        return true;
    }

    return false;
}

bool pattern_repeat_is_start(passgen_token_t token) {
    return passgen_token_is_regular(&token) && token.codepoint == '{';
}

bool pattern_repeat_is_sep(passgen_token_t token) {
    return passgen_token_is_regular(&token) && token.codepoint == ',';
}

bool pattern_repeat_is_end(passgen_token_t token) {
    return passgen_token_is_regular(&token) && token.codepoint == '}';
}

static pattern_result_t
pattern_parse_repeat(
        pattern_repeat_t *repeat,
        unicode_iter_t *iter)
{
    unicode_iter_t prev = *iter;
    pattern_result_t result = result_ok;

    passgen_token_t token = passgen_token_next(iter);

    // don't do anything if this isn't a start token.
    if(!pattern_repeat_is_start(token)) {
        *iter = prev;
        repeat->min = 1;
        repeat->max = 1;
        return result;
    }

    // parse min
    result = passgen_parse_number(&repeat->min, iter);
    if(!result.ok) {
        return result;
    }

    token = passgen_token_next(iter);

    if(pattern_repeat_is_sep(token)) {
        // parse max
        result = passgen_parse_number(&repeat->max, iter);
        if(!result.ok) {
            return result;
        }
    } else {
        repeat->max = repeat->min;
    }

    // check if comma or end brace is next
    token = passgen_token_next(iter);

    if(!pattern_repeat_is_end(token)) {
        // error
    }

    if(repeat->min > repeat->max) {
        // error
    }

    return result;
}

static inline pattern_result_t
passgen_parse_number(
        size_t *number,
        unicode_iter_t *iter)
{
    unicode_iter_t prev = *iter;
    passgen_token_t token;
    size_t num = 0;
    size_t len = 0;

    token = passgen_token_next(iter);
    while(true) {
        if(!passgen_token_is_regular(&token)) {
            break;
        }

        if('0' <= token.codepoint && token.codepoint <= '9') {
            num *= 10;
            num += token.codepoint - '0';
        } else {
            break;
        }

        len += 1;
        token = passgen_token_next(iter);
    }

    if(len == 0) {
        // error
    }

    if(passgen_token_is_error(&token)) {
        // error
    }

    // that last thing we aren't interested in.
    *iter = prev;

    // store result
    *number = num;
    return result_ok;
}

/*
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

pattern_ranges_t *pattern_ranges_new(
    char start,
    char end,
    pattern_ranges_t *next) {
  pattern_ranges_t *new = malloc(sizeof(pattern_ranges_t));
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

pattern_ranges_t *pattern_ranges_parse(const char **string) {
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

  return pattern_ranges_new(start, end, pattern_ranges_parse(string));
}

void pattern_ranges_free(pattern_ranges_t *range) {
  if(!range) return;

  if(range->next) {
    pattern_ranges_free(range->next);
  }

  free(range);
}

char pattern_ranges_random(pattern_ranges_t *range, random_t *rand) {
  // calculate sum of possible chars
  size_t poss = 0;
  for(pattern_ranges_t *pos = range; pos != NULL; pos = pos->next) {
    poss += pos->end - pos->start + 1;
  }

  size_t choice = random_uint64_max(rand, poss);

  for(pattern_ranges_t *pos = range; pos != NULL; pos = pos->next) {
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

pattern_segments_t *pattern_segments_parse(const char **string) {
  if(is_end(**string) || is_sep(**string) || is_illegal(**string)) {
    // TODO: empty segment.
    pattern_reps_t reps = { .min = 0, .max = 0 };
    return pattern_segments_new(PATTERN_CHAR, (void *)*string, reps, NULL);
  }

  // parse the kind and data.
  pattern_kind kind;
  void *data = NULL;
  switch(**string) {
    case '[':
      *string += 1;
      kind = PATTERN_RANGE;
      data = pattern_ranges_parse(string);
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
    reps = pattern_segments_parse_reps(string);
    if(**string != '}') goto fail;
    *string += 1;
  }

  // don't recurse if we're at the end.
  if(is_end(**string) || is_sep(**string) || is_illegal(**string)) {
    return pattern_segments_new(kind, data, reps, NULL);
  }

  return pattern_segments_new(kind, data, reps, pattern_segments_parse(string));
fail:
  if(data) {
    switch(kind) {
      case PATTERN_GROUP:
        pattern_free(data);
        break;
      case PATTERN_RANGE:
        pattern_ranges_free(data);
        break;
      case PATTERN_CHAR:
        break;
    }
  }

  return NULL;
}

pattern_segments_t *pattern_segments_new(
    pattern_kind kind,
    void *data,
    pattern_reps_t reps,
    pattern_segments_t *next) {
  pattern_segments_t *pattern = malloc(sizeof(pattern_segments_t));
  assert(pattern);

  pattern->kind = kind;
  pattern->data.raw = data;
  pattern->reps = reps;
  pattern->next = next;

  return pattern;
}

void pattern_segments_free(pattern_segments_t *pattern) {
  if(!pattern) return;

  switch(pattern->kind) {
    case PATTERN_RANGE:
      pattern_ranges_free(pattern->data.range);
      break;
    case PATTERN_GROUP:
      pattern_free(pattern->data.group);
    case PATTERN_CHAR:
      break;
  }

  if(pattern->next) {
    pattern_segments_free(pattern->next);
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

pattern_reps_t pattern_segments_parse_reps(const char **string) {
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

size_t pattern_segments_maxlen(pattern_segments_t *pattern) {
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
    return len + pattern_segments_maxlen(pattern->next);
  }
}

size_t pattern_segments_random_fill(
    pattern_segments_t *segment,
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
          str[written] = pattern_ranges_random(segment->data.range, rand);
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

char *pattern_segments_random(pattern_segments_t *pattern, random_t *rand) {
  // create buffer that is big enough.
  size_t len = pattern_segments_maxlen(pattern);
  char *buffer = malloc(len + 1);
  assert(buffer);

  // write.
  size_t written = pattern_segments_random_fill(pattern, rand, buffer, len);

  // null-terminate.
  buffer[written] = '\0';

  return buffer;
}

pattern_t *pattern_new(pattern_segments_t *segment, pattern_t *next) {
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
    pattern_segments_free(pattern->item);
    next = pattern->next;
    free(pattern);
    pattern = next;
  }
}

pattern_t *pattern_parse(const char **string) {
  pattern_t *pattern = pattern_new(pattern_segments_parse(string), NULL);

  pattern_t *rest = pattern;
  while(!is_illegal(**string) && !is_end(**string)) {
    *string += 1;
    rest->next = pattern_new(pattern_segments_parse(string), NULL);
    rest = rest->next;
  }

  return pattern;
}

size_t pattern_maxlen(pattern_t *pattern) {
  size_t maxlen = 0;

  // return the largest segment's maxlen.
  while(pattern) {
    size_t maxlen_cur = pattern_segments_maxlen(pattern->item);
    if(maxlen_cur > maxlen) maxlen = maxlen_cur;
  }

  return maxlen;
}

size_t pattern_segments_count(pattern_t *pattern) {
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
  size_t segments = pattern_segments_count(pattern);
  size_t choice = random_uint64_max(rand, segments);

  // find segment
  while(choice) {
    pattern = pattern->next;
    choice -= 1;
  }

  return pattern_segments_random_fill(pattern->item, rand, buffer, len);
}

char *pattern_random(pattern_t *pattern, random_t *rand) {
  // choose a segment
  size_t segments = pattern_segments_count(pattern);
  size_t choice = random_uint64_max(rand, segments);

  // find segment
  while(choice) {
    pattern = pattern->next;
    choice -= 1;
  }

  return pattern_segments_random(pattern->item, rand);
}

size_t pattern_choices(pattern_t *pattern) {
  size_t choices = 0;

  while(pattern) {
    choices += pattern_segments_choices(pattern->item);
    pattern = pattern->next;
  }

  return choices;
}

size_t pattern_ranges_choices(pattern_ranges_t *range) {
  size_t choices = 0;

  while(range) {
    choices += range->end - range->start + 1;
    range = range->next;
  }

  return choices;
}

size_t pattern_segments_choices(pattern_segments_t *segment) {
  size_t choices = 0;

  while(segment) {
    size_t lchoices;
    switch(segment->kind) {
      case PATTERN_CHAR:
        lchoices = 1;
        break;
      case PATTERN_RANGE:
        lchoices = pattern_ranges_choices(segment->data.range);
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
*/

