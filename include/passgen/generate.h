#pragma once
#include "passgen/data/pattern.h"
#include "passgen/pronounceable.h"

struct passgen_pattern_repeat;
struct passgen_pattern_set;
struct passgen_pattern_char;
struct passgen_pattern_group;
struct passgen_pattern_item;
struct passgen_pattern_segment;
struct passgen_pattern_special;

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

size_t passgen_generate_repeat(
    random_t *rand,
    struct pattern_env *env,
    struct passgen_pattern_repeat *repeat);

int passgen_generate_set(
    struct passgen_pattern_set *set,
    random_t *rand,
    struct pattern_env *env,
    void *data,
    passgen_generate_cb *func);

int passgen_generate_character(
    struct passgen_pattern_char *character,
    random_t *rand,
    struct pattern_env *env,
    void *data,
    passgen_generate_cb *func);

int passgen_generate_special_pronounceable(
    struct passgen_pattern_special *special,
    random_t *rand,
    struct pattern_env *env,
    void *data,
    passgen_generate_cb *func);

int passgen_generate_special_wordlist(
    struct passgen_pattern_special *special,
    random_t *rand,
    struct pattern_env *env,
    void *data,
    passgen_generate_cb *func);

int passgen_generate_special(
    struct passgen_pattern_special *special,
    random_t *rand,
    struct pattern_env *env,
    void *data,
    passgen_generate_cb *func);

int passgen_generate_group(
    struct passgen_pattern_group *group,
    random_t *rand,
    struct pattern_env *env,
    void *data,
    passgen_generate_cb *func);

int passgen_generate_item(
    struct passgen_pattern_item *item,
    random_t *rand,
    struct pattern_env *env,
    void *data,
    passgen_generate_cb *func);

int passgen_generate_segment(
    struct passgen_pattern_segment *segment,
    random_t *rand,
    struct pattern_env *env,
    void *data,
    passgen_generate_cb *func);
