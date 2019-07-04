#include <stdio.h>
#include <assert.h>
#include <stdbool.h>
#include "random.h"
#include "pattern.h"

void test_random();
void test_random_uint8();
void test_random_uint8_max();
void test_pattern();
void test_pattern_range();
void test_pattern_range_range();
void test_pattern_range_char();
void test_pattern_range_combined();
void test_pattern_range_err();
void test_pattern_parse();
void test_pattern_parse_reps();

int main(int argc, char *argv[]) {
  test_random();
  test_pattern();

  fprintf(stdout, "all tests passed.\n");
  return 0;
}

void test_random() {
  test_random_uint8();
  test_random_uint8_max();
}

void test_pattern() {
  test_pattern_range();
  test_pattern_parse();
}

void test_pattern_parse() {
  test_pattern_parse_reps();
}

void test_pattern_parse_reps() {
  const char *s = "{123,234}";
  const char *p = &s[1];
  pattern_reps_t reps = pattern_parse_reps(&p);
  assert(*p == '}');
  assert(reps.min == 123);
  assert(reps.max == 234);

  s = "{991}";
  p = &s[1];
  reps = pattern_parse_reps(&p);
  assert(*p == '}');
  assert(reps.min == 991);
  assert(reps.max == 991);
}

void test_pattern_range() {
  test_pattern_range_range();
  test_pattern_range_char();
  test_pattern_range_combined();
  test_pattern_range_err();
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

void test_random_uint8() {
  random_t *rand = random_new();
  assert(rand);

  // generate random nubers until we got almost all of them.
  bool gotten[UINT8_MAX] = {false};
  for(size_t i = 0; i < (32 * UINT8_MAX); ++i) {
    gotten[random_uint8(rand)] = true;
  }

  // there's still a (255/256)^(8*256) = 0.03% chance this fails.
  for(size_t i = 0; i < UINT8_MAX; ++i) {
    assert(gotten[i]);
  }

  random_close(rand);
}

void test_random_uint8_max() {
  random_t *rand = random_new();
  assert(rand);

  for(size_t max = 1; max < UINT8_MAX; ++max) {
    // generate random nubers until we got almost all of them.
    bool gotten[UINT8_MAX] = {false};
    for(size_t i = 0; i < (16 * UINT8_MAX); ++i) {
      uint8_t r = random_uint8_max(rand, max);
      assert(r <= max);
      gotten[r] = true;
    }

    // there's still a (255/256)^(8*256) = 0.03% chance this fails.
    for(size_t i = 0; i < max; ++i) {
      assert(gotten[i]);
    }
  }

  random_close(rand);
}
