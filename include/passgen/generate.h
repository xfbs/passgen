#pragma once
#include "passgen/data/pattern.h"
#include "passgen/pronounceable.h"

struct pattern_env {
  bool find_complexity;
  double complexity;
  size_t pronounceable_limit;
  enum passgen_pronounceable_type pronounceable_type;
};

typedef int passgen_generate_cb(void *data, int32_t codepoint);

size_t passgen_generate_fill(
    struct passgen_pattern *pattern,
    random_t *rand,
    struct pattern_env *env,
    char *buffer,
    size_t len);

int passgen_generate(
    struct passgen_pattern *pattern,
    random_t *rand,
    struct pattern_env *env,
    void *data,
    passgen_generate_cb *func);
