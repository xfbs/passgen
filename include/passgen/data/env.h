#pragma once
#include "passgen/hashmap.h"
#include "passgen/random.h"

struct passgen_env {
  bool find_complexity;
  double complexity;
  passgen_hashmap wordlists;
  passgen_random_t *random;
};

