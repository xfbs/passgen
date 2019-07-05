#include "passgen/pattern.h"
#include "passgen/random.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

void test_pattern_parse();
void test_pattern_range();
void test_pattern_range_range();
void test_pattern_range_char();
void test_pattern_range_combined();
void test_pattern_range_err();
void test_pattern_range_random();
void test_pattern_segment();
void test_pattern_segment_parse();
void test_pattern_segment_parse_chars();
void test_pattern_segment_parse_range();
void test_pattern_segment_parse_group();
void test_pattern_segment_parse_reps();
void test_pattern_segment_maxlen();
void test_pattern_segment_random();

int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;

  test_pattern_parse();
  test_pattern_range();
  test_pattern_segment();

  fprintf(stdout, "all tests passed.\n");
  return 0;
}

void test_pattern_parse() {
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
}

void test_pattern_segment() {
  test_pattern_segment_parse();
  test_pattern_segment_maxlen();
  test_pattern_segment_random();
}

void test_pattern_segment_random() {
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
}

void test_pattern_segment_parse() {
  test_pattern_segment_parse_chars();
  test_pattern_segment_parse_range();
  test_pattern_segment_parse_group();
  test_pattern_segment_parse_reps();
}

void test_pattern_segment_maxlen() {
  const char *s = "a\\({2}b{11,12}";
  pattern_segment_t *pattern = pattern_segment_parse(&s);
  assert(pattern_segment_maxlen(pattern) == 15);
  pattern_segment_free(pattern);

  s = "a{2,3}[a-z]{5}\\]{2}";
  pattern = pattern_segment_parse(&s);
  assert(pattern_segment_maxlen(pattern) == 10);
  pattern_segment_free(pattern);
}

void test_pattern_segment_parse_chars() {
  const char *s = "a\\({2}b{11,12}";
  const char *p = s;
  pattern_segment_t *pattern = pattern_segment_parse(&p);
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
}

void test_pattern_segment_parse_range() {
  const char *s = "[a-z]{12,13}";
  const char *p = s;
  pattern_segment_t *pattern = pattern_segment_parse(&p);
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
}

void test_pattern_segment_parse_group() {
  // TODO
}

void test_pattern_segment_parse_reps() {
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
}

void test_pattern_range() {
  test_pattern_range_range();
  test_pattern_range_char();
  test_pattern_range_combined();
  test_pattern_range_err();
  test_pattern_range_random();
}

void test_pattern_range_random() {
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
}

void test_pattern_range_range() {
  const char *s = "[a-z]";
  const char *p = &s[1];
  pattern_range_t *range = pattern_range_parse(&p);
  assert(range);
  assert(p == &s[4]);
  assert(range->start == 'a');
  assert(range->end == 'z');
  assert(range->next == NULL);
  pattern_range_free(range);
}

void test_pattern_range_char() {
  const char *s = "[a]";
  const char *p = &s[1];
  pattern_range_t *range = pattern_range_parse(&p);
  assert(range);
  assert(p == &s[2]);
  assert(range->start == 'a');
  assert(range->end == 'a');
  assert(range->next == NULL);
  pattern_range_free(range);
}

void test_pattern_range_combined() {
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
}

void test_pattern_range_err() {
  const char *s = "]";
  const char *p = s;
  assert(!pattern_range_parse(&p));
  assert(p == s);

  p = s = ")";
  assert(!pattern_range_parse(&p));
  assert(p == s);

  p = s = "a-";
  assert(!pattern_range_parse(&p));
  assert(p == s);

  p = s = "";
  assert(!pattern_range_parse(&p));
  assert(p == s);
}
