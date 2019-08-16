#include "passgen/pattern.h"
#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <regex.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <jansson.h>
#include <stdbool.h>

typedef void bench_func(void *data);

struct bench_options {
  double time;
  size_t threads;
  regex_t *preg;
  bool json;
};

struct bench_item {
  const char *name;
  bench_func *func;
  void *data;
};

struct bench_result {
  size_t amount;
  clock_t *time;
};

struct bench_times {
  double mean;
  double median;
  double min;
  double max;
  double dev;
};

struct bench_state {
  size_t item_count;
  clock_t target;
  const struct bench_options *opts;
  const struct bench_item *items;
  struct bench_result *results;

  atomic_size_t pos;
};

struct bench_item items[];

void *bench_thread(void *data) {
  struct bench_state *state = data;

  // fetch the next item.
  size_t cur;
  while((cur = atomic_fetch_add(&state->pos, 1)) < state->item_count) {
    struct bench_result *result = &state->results[cur];
    const struct bench_item *item = &state->items[cur];

    clock_t total = 0;
    result->amount = 0;

    size_t capacity = 1000;
    result->time = malloc(capacity * sizeof(clock_t));
    assert(result->time);

    while(total < state->target) {
      // run benchmark
      clock_t before = clock();
      item->func(item->data);
      clock_t after = clock();

      // save time
      result->time[result->amount] = after - before;
      total += after - before;

      // save amount
      result->amount += 1;

      // make space
      if(result->amount == capacity) {
        capacity *= 3;
        result->time = realloc(result->time, capacity * sizeof(clock_t));
        assert(result->time);
      }
    }
  }

  return NULL;
}

int compare_clock(const void *a, const void *b) {
  const clock_t *lhs = a;
  const clock_t *rhs = b;

  if(*lhs < *rhs) {
    return -1;
  } else if(*lhs > *rhs) {
    return 1;
  } else {
    return 0;
  }
}

struct bench_times bench_process(const struct bench_result *result) {
  struct bench_times times = {
    .mean = 0,
    .median = 0,
    .min = result->time[0],
    .max = result->time[0],
    .dev = 0,
  };

  // find mean, min and max
  for(size_t i = 0; i < result->amount; i++) {
    clock_t time = result->time[i];

    times.mean += time;

    if(time < times.min) {
      times.min = time;
    }

    if(time > times.max) {
      times.max = time;
    }
  }

  // find real mean
  times.mean /= result->amount;

  // find stddev
  for(size_t i = 0; i < result->amount; i++) {
    clock_t time = result->time[i];

    times.dev += (time - times.mean) * (time - times.mean);
  }

  times.dev /= result->amount;
  times.dev = sqrt(times.dev);

  // find median by copying data first
  clock_t *copy = malloc(result->amount * sizeof(clock_t));
  assert(copy);
  memcpy(copy, result->time, result->amount * sizeof(clock_t));
  qsort(copy, result->amount, sizeof(clock_t), &compare_clock);
  times.median = copy[result->amount / 2];
  free(copy);

  times.mean = times.mean / CLOCKS_PER_SEC;
  times.median = times.median / CLOCKS_PER_SEC;
  times.min = times.min / CLOCKS_PER_SEC;
  times.max = times.max / CLOCKS_PER_SEC;
  times.dev /= CLOCKS_PER_SEC;

  return times;
}

void bench_output_json(const struct bench_state *state) {
  json_t *output = json_object();
  json_object_set_new(output, "time", json_real(state->opts->time));
  json_object_set_new(output, "threads", json_integer(state->opts->threads));
  json_t *results = json_array();
  json_object_set_new(output, "results", results);

  for(size_t i = 0; i < state->item_count; i++) {
    const struct bench_item *item = &state->items[i];
    const struct bench_result *res = &state->results[i];
    struct bench_times times = bench_process(res);

    json_t *result = json_object();
    json_object_set_new(result, "name", json_string(item->name));
    json_object_set_new(result, "amount", json_integer(res->amount));
    json_object_set_new(result, "mean", json_real(times.mean));
    json_object_set_new(result, "median", json_real(times.median));
    json_object_set_new(result, "min", json_real(times.min));
    json_object_set_new(result, "max", json_real(times.max));
    json_object_set_new(result, "dev", json_real(times.dev));
    json_array_append_new(results, result);
  }

  char *str = json_dumps(output, JSON_INDENT(4));
  printf("%s\n", str);
  free(str);
  json_decref(output);
}

void bench(const struct bench_options *opts, const struct bench_item *items) {
  struct bench_state state = {
    .item_count = 0,
    .target = opts->time * CLOCKS_PER_SEC,
    .opts = opts,
    .items = items,
    .results = NULL,
  };

  // find out how many we have to test.
  while(items[state.item_count].name) {
    state.item_count += 1;
  }

  // allocate memory for the results.
  state.results = calloc(state.item_count, sizeof(struct bench_result));
  assert(state.results);

  // init state
  atomic_store(&state.pos, 0);

  // initialise threads
  pthread_t threads[opts->threads];
  for(size_t i = 0; i < opts->threads; i++) {
    pthread_create(&threads[i], NULL, bench_thread, &state);
  }

  // wait for threads to finish
  for(size_t i = 0; i < opts->threads; i++) {
    pthread_join(threads[i], NULL);
  }

  if(opts->json) {
    bench_output_json(&state);
  } else {
    // print stats
    for(size_t i = 0; i < state.item_count; i++) {
      struct bench_times times = bench_process(&state.results[i]);

      printf("%s got %zu iterations\n", items[i].name, state.results[i].amount);
      printf("  mean %f median %f min %f max %f dev %f\n",
          1000 * times.mean,
          1000 * times.median,
          1000 * times.min,
          1000 * times.max,
          1000 * times.dev);
    }
  }

  // clean up memory
  for(size_t i = 0; i < state.item_count; i++) {
    free(state.results[i].time);
  }

  free(state.results);
}

int main() {
  struct bench_options opts = {
    .time = 0.05,
    .threads = 1,
    .preg = NULL,
    .json = false,
  };

  struct bench_item items[] = {
    {"pattern_parse", &bench_pattern_parse, "aaa"},
    {"pattern_parse", &bench_pattern_parse, "aaa"},
    {NULL, NULL, NULL},
  };

  bench(&opts, items);

  return 0;
}

void bench_pattern_parse(void *data) {
  char *str = data;

  pattern_t *pattern = pattern_parse(&str);
  assert(pattern);
  pattern_free(pattern);
}
