#include "passgen/pattern.h"
#include "passgen/random.h"
#include "util/tests.h"
#include <stdbool.h>
#include <stdio.h>

test_ret test_pattern_parse() {
  const char *s = "abc|def";
  pattern_t *pattern = pattern_parse(&s);
  assert(pattern);
  assert(pattern->item->kind == PATTERN_CHAR);
  assert(pattern->item->data.chr[0] == 'a');
  assert(pattern->item->next->kind == PATTERN_CHAR);
  assert(pattern->item->next->data.chr[0] == 'b');
  assert(pattern->item->next->next->kind == PATTERN_CHAR);
  assert(pattern->item->next->next->data.chr[0] == 'c');
  assert(!pattern->item->next->next->next);
  assert(pattern->next->item->kind == PATTERN_CHAR);
  assert(pattern->next->item->data.chr[0] == 'd');
  assert(pattern->next->item->next->kind == PATTERN_CHAR);
  assert(pattern->next->item->next->data.chr[0] == 'e');
  assert(pattern->next->item->next->next->kind == PATTERN_CHAR);
  assert(pattern->next->item->next->next->data.chr[0] == 'f');
  assert(!pattern->next->item->next->next->next);
  assert(!pattern->next->next);
  pattern_free(pattern);

  s = "a";
  pattern = pattern_parse(&s);
  assert(pattern);
  assert(!pattern->next);
  assert(pattern->item);
  assert(pattern->item->kind == PATTERN_CHAR);
  assert(pattern->item->data.chr[0] == 'a');
  assert(!pattern->item->next);
  pattern_free(pattern);

  s = "";
  pattern = pattern_parse(&s);
  assert(pattern);
  assert(!pattern->next);
  assert(pattern->item);
  pattern_free(pattern);

  return test_ok;
}

test_ret test_pattern_segment_random() {
  const char *s = "a\\({2}[ab]{10,12}";
  pattern_segment_t *pattern = pattern_segment_parse(&s);
  random_t *rand = random_new();
  const char *out = pattern_segment_random(pattern, rand);
  assert(out);
  assert(out[0] == 'a');
  assert(out[1] == '(');
  assert(out[2] == '(');
  assert(out[3] == 'a' || out[3] == 'b');
  assert(out[4] == 'a' || out[4] == 'b');
  assert(out[5] == 'a' || out[5] == 'b');
  assert(out[6] == 'a' || out[6] == 'b');
  assert(out[7] == 'a' || out[7] == 'b');
  assert(out[8] == 'a' || out[8] == 'b');
  assert(out[9] == 'a' || out[9] == 'b');
  assert(out[10] == 'a' || out[10] == 'b');
  assert(out[11] == 'a' || out[11] == 'b');
  assert(out[12] == 'a' || out[12] == 'b');
  assert(out[13] == 'a' || out[13] == 'b' || out[13] == '\0');
  assert(out[14] == 'a' || out[14] == 'b' || out[14] == '\0');
  free((void *)out);

  pattern_segment_free(pattern);
  random_close(rand);

  return test_ok;
}

test_ret test_pattern_segment_maxlen() {
  const char *s = "a\\({2}b{11,12}";
  pattern_segment_t *pattern = pattern_segment_parse(&s);
  assert(pattern_segment_maxlen(pattern) == 15);
  pattern_segment_free(pattern);

  s = "a{2,3}[a-z]{5}\\]{2}";
  pattern = pattern_segment_parse(&s);
  assert(pattern_segment_maxlen(pattern) == 10);
  pattern_segment_free(pattern);

  return test_ok;
}

test_ret test_pattern_segment_parse_chars() {
  const char *s;
  const char *p;
  pattern_segment_t *pattern;

  p = s = "a";
  pattern = pattern_segment_parse(&p);
  assert(pattern);
  assert(!pattern->next);
  assert(pattern->kind == PATTERN_CHAR);
  assert(pattern->data.chr[0] == 'a');
  assert(*p == '\0');
  pattern_segment_free(pattern);

  // stop at sep
  p = s = "a|";
  pattern = pattern_segment_parse(&p);
  assert(pattern);
  assert(!pattern->next);
  assert(pattern->kind == PATTERN_CHAR);
  assert(pattern->data.chr[0] == 'a');
  assert(pattern->reps.min == 1);
  assert(pattern->reps.max == 1);
  assert(*p == '|');
  pattern_segment_free(pattern);

  // stop at illegal
  p = s = "a)";
  pattern = pattern_segment_parse(&p);
  assert(pattern);
  assert(!pattern->next);
  assert(pattern->kind == PATTERN_CHAR);
  assert(pattern->data.chr[0] == 'a');
  assert(pattern->reps.min == 1);
  assert(pattern->reps.max == 1);
  assert(*p == ')');
  pattern_segment_free(pattern);

  // stop at illegal
  p = s = "a]";
  pattern = pattern_segment_parse(&p);
  assert(pattern);
  assert(!pattern->next);
  assert(pattern->kind == PATTERN_CHAR);
  assert(pattern->data.chr[0] == 'a');
  assert(pattern->reps.min == 1);
  assert(pattern->reps.max == 1);
  assert(*p == ']');
  pattern_segment_free(pattern);

  // parse escaped
  p = s = "\\]";
  pattern = pattern_segment_parse(&p);
  assert(pattern);
  assert(!pattern->next);
  assert(pattern->kind == PATTERN_CHAR);
  assert(pattern->data.chr[0] == ']');
  assert(pattern->reps.min == 1);
  assert(pattern->reps.max == 1);
  assert(*p == '\0');
  pattern_segment_free(pattern);

  // parse repetitons.
  p = s = "a{12}";
  pattern = pattern_segment_parse(&p);
  assert(pattern);
  assert(!pattern->next);
  assert(pattern->kind == PATTERN_CHAR);
  assert(pattern->data.chr[0] == 'a');
  assert(pattern->reps.min == 12);
  assert(pattern->reps.max == 12);
  assert(*p == '\0');
  pattern_segment_free(pattern);

  // parse repetitons.
  p = s = "a{12,23}";
  pattern = pattern_segment_parse(&p);
  assert(pattern);
  assert(!pattern->next);
  assert(pattern->kind == PATTERN_CHAR);
  assert(pattern->data.chr[0] == 'a');
  assert(pattern->reps.min == 12);
  assert(pattern->reps.max == 23);
  assert(*p == '\0');
  pattern_segment_free(pattern);

  // parse complex.
  p = s = "a\\({2}b{11,12}";
  pattern = pattern_segment_parse(&p);
  assert(pattern);
  assert(pattern->kind == PATTERN_CHAR);
  assert(pattern->data.chr == &s[0]);
  assert(pattern->reps.min == 1);
  assert(pattern->reps.max == 1);
  assert(pattern->next);
  assert(pattern->next->kind == PATTERN_CHAR);
  assert(pattern->next->data.chr == &s[2]);
  assert(pattern->next->reps.min == 2);
  assert(pattern->next->reps.max == 2);
  assert(pattern->next->next);
  assert(pattern->next->next->kind == PATTERN_CHAR);
  assert(pattern->next->next->data.chr == &s[6]);
  assert(pattern->next->next->reps.min == 11);
  assert(pattern->next->next->reps.max == 12);
  assert(!pattern->next->next->next);
  assert(p == &s[14]);
  pattern_segment_free(pattern);

  return test_ok;
}

test_ret test_pattern_segment_parse_range() {
  const char *s;
  const char *p;
  pattern_segment_t *pattern;

  p = s = "[a-z]{12,13}";
  pattern = pattern_segment_parse(&p);
  assert(pattern);
  assert(pattern->kind == PATTERN_RANGE);
  assert(pattern->data.range);
  assert(pattern->data.range->start == 'a');
  assert(pattern->data.range->end == 'z');
  assert(pattern->data.range->next == NULL);
  assert(pattern->reps.min == 12);
  assert(pattern->reps.max == 13);
  assert(!pattern->next);
  assert(p == &s[12]);
  pattern_segment_free(pattern);

  return test_ok;
}

test_ret test_pattern_segment_parse_err() {
  const char *s;
  const char *p;
  pattern_segment_t *pattern;

  p = s = "[a-z){12,13}";
  pattern = pattern_segment_parse(&p);
  assert(!pattern);

  p = s = "(a-z]{12,13}";
  pattern = pattern_segment_parse(&p);
  assert(!pattern);

  return test_ok;
}

test_ret test_pattern_segment_parse_group() {
  const char *s = "(abc){2,3}";
  const char *p = s;
  pattern_segment_t *pattern = pattern_segment_parse(&p);
  assert(pattern);
  assert(pattern->kind == PATTERN_GROUP);
  assert(pattern->reps.min == 2);
  assert(pattern->reps.max == 3);
  assert(!pattern->next);
  assert(*p == '\0');
  pattern_segment_free(pattern);

  return test_ok;
}

test_ret test_pattern_segment_parse_reps() {
  const char *s = "{123,234}";
  const char *p = &s[1];
  pattern_reps_t reps = pattern_segment_parse_reps(&p);
  assert(*p == '}');
  assert(reps.min == 123);
  assert(reps.max == 234);

  s = "{991}";
  p = &s[1];
  reps = pattern_segment_parse_reps(&p);
  assert(*p == '}');
  assert(reps.min == 991);
  assert(reps.max == 991);

  return test_ok;
}

test_ret test_pattern_range_random() {
  random_t *rand = random_new();
  const char *s;
  pattern_range_t *range;
  s = "ace0-1";
  range = pattern_range_parse(&s);

  for (size_t i = 0; i < 256; i++) {
    char c = pattern_range_random(range, rand);
    assert(c == 'a' || c == 'c' || c == 'e' || c == '0' || c == '1');
  }

  pattern_range_free(range);
  random_close(rand);

  return test_ok;
}

test_ret test_pattern_range_range() {
  const char *s = "[a-z]";
  const char *p = &s[1];
  pattern_range_t *range = pattern_range_parse(&p);
  assert(range);
  assert(p == &s[4]);
  assert(range->start == 'a');
  assert(range->end == 'z');
  assert(range->next == NULL);
  pattern_range_free(range);

  return test_ok;
}

test_ret test_pattern_range_char() {
  const char *s = "[a]";
  const char *p = &s[1];
  pattern_range_t *range = pattern_range_parse(&p);
  assert(range);
  assert(p == &s[2]);
  assert(range->start == 'a');
  assert(range->end == 'a');
  assert(range->next == NULL);
  pattern_range_free(range);

  return test_ok;
}

test_ret test_pattern_range_combined() {
  const char *s = "[a-z0-9%#]";
  const char *p = &s[1];
  pattern_range_t *range = pattern_range_parse(&p);
  assert(range);
  assert(p == &s[9]);
  assert(range->start == 'a');
  assert(range->end == 'z');
  assert(range->next);
  assert(range->next->start == '0');
  assert(range->next->end == '9');
  assert(range->next->next != NULL);
  assert(range->next->next->start == '%');
  assert(range->next->next->end == '%');
  assert(range->next->next->next != NULL);
  assert(range->next->next->next->start == '#');
  assert(range->next->next->next->end == '#');
  assert(range->next->next->next->next == NULL);
  pattern_range_free(range);

  return test_ok;
}

test_ret test_pattern_range_err() {
  const char *s = "]";
  const char *p = s;
  assert(!pattern_range_parse(&p));
  assert(p == s);

  p = s = ")";
  assert(!pattern_range_parse(&p));
  assert(p == s);

  p = s = "a-";
  assert(!pattern_range_parse(&p));

  p = s = "";
  assert(!pattern_range_parse(&p));
  assert(p == s);

  return test_ok;
}

test_ret test_pattern_range_choices() {
  // single char.
  pattern_range_t *range = pattern_range_new('a', 'a', NULL);
  assert(range);
  assert(pattern_range_choices(range) == 1);
  pattern_range_free(range);

  // char range.
  range = pattern_range_new('a', 'c', NULL);
  assert(range);
  assert(pattern_range_choices(range) == 3);
  pattern_range_free(range);

  // multi chars.
  range = pattern_range_new('a', 'a', NULL);
  assert(range);
  range = pattern_range_new('c', 'c', range);
  assert(range);
  range = pattern_range_new('e', 'e', range);
  assert(range);
  assert(pattern_range_choices(range) == 3);
  pattern_range_free(range);

  // multi char range.
  range = pattern_range_new('a', 'c', NULL);
  assert(range);
  range = pattern_range_new('e', 'f', range);
  assert(range);
  range = pattern_range_new('h', 'k', range);
  assert(range);
  assert(pattern_range_choices(range) == (3 + 2 + 4));
  pattern_range_free(range);

  return test_ok;
}

test_ret test_pattern_segment_choices() {
  pattern_segment_t *segment;
  pattern_range_t *range;

  // single char.
  segment = pattern_segment_new(PATTERN_CHAR, NULL, (pattern_reps_t){.min = 1, .max = 1}, NULL);
  assert(segment);
  assert(pattern_segment_choices(segment) == 1);
  pattern_segment_free(segment);

  // single char, variable length.
  segment = pattern_segment_new(PATTERN_CHAR, NULL, (pattern_reps_t){.min = 5, .max = 8}, NULL);
  assert(segment);
  assert(pattern_segment_choices(segment) == 4); // 5, 6, 7, or 8 repetitions.
  pattern_segment_free(segment);

  // range.
  range = pattern_range_new('a', 'c', NULL);
  assert(range);
  segment = pattern_segment_new(PATTERN_RANGE, range, (pattern_reps_t){.min = 8, .max = 8}, NULL);
  assert(segment);
  assert(pattern_range_choices(range) == 3);
  assert(pattern_segment_choices(segment) == 3); // 5, 6, 7, or 8 repetitions.
  pattern_range_free(range);
  pattern_segment_free(segment);

  // range, variable repetitions.
  range = pattern_range_new('a', 'c', NULL);
  assert(range);
  segment = pattern_segment_new(PATTERN_RANGE, range, (pattern_reps_t){.min = 5, .max = 8}, NULL);
  assert(segment);
  assert(pattern_range_choices(range) == 3);
  assert(pattern_segment_choices(segment) == (4 * 3)); // 5, 6, 7, or 8 repetitions.
  pattern_range_free(range);
  pattern_segment_free(segment);

  // TODO: test group.

  return test_ok;
}

test_t tests[] = {
  test(pattern_parse),
  test(pattern_range_range),
  test(pattern_range_char),
  test(pattern_range_combined),
  test(pattern_range_err),
  test(pattern_range_random),
  test(pattern_range_choices),
  test(pattern_segment_parse_chars),
  test(pattern_segment_parse_range),
  test(pattern_segment_parse_group),
  test(pattern_segment_parse_reps),
  test(pattern_segment_parse_err),
  test(pattern_segment_maxlen),
  test(pattern_segment_random),
  test(pattern_segment_choices),
  {NULL, NULL}
};
