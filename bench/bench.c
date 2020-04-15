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
#include <getopt.h>
#include <_regex.h>
#include <stddef.h>
#include <sys/_pthread/_pthread_t.h>

#include "bench.h"

struct bench_options {
  double time;
  size_t threads;
  regex_t *regex;
  bool json;
  bool help;
  bool version;
  bool error;
  bool list;
};

struct bench_result {
  bool enabled;
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


void *bench_thread(void *data) {
  struct bench_state *state = data;

  // fetch the next item.
  size_t cur;
  while((cur = atomic_fetch_add(&state->pos, 1)) < state->item_count) {
    struct bench_result *result = &state->results[cur];
    const struct bench_item *item = &state->items[cur];

    if(!result->enabled) {
      continue;
    }

    clock_t total = 0;
    result->amount = 0;

    size_t capacity = 1000;
    result->time = malloc(capacity * sizeof(clock_t));
    assert(result->time);

    // setup state.
    void *data = NULL;
    if(item->setup) {
      data = item->setup(NULL);
    }

    while(total < state->target) {
      // run benchmark
      clock_t before = clock();
      item->func(data);
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

    // clean up state
    if(item->free) {
      item->free(data);
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

    if(!res->enabled) {
      continue;
    }

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

  // enable all tests that match the regex if one exists.
  if(opts->regex) {
    for(size_t i = 0; i < state.item_count; i++) {
      if(0 == regexec(opts->regex, items[i].name, 0, NULL, 0)) {
        state.results[i].enabled = true;

        printf("enabling %s\n", items[i].name);
      }
    }
  } else {
    // enable all
    for(size_t i = 0; i < state.item_count; i++) {
      state.results[i].enabled = true;
    }
  }

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
      const struct bench_result *res = &state.results[i];

      if(!res->enabled) {
        continue;
      }

      struct bench_times times = bench_process(res);

      printf("%s got %zu iterations\n", items[i].name, res->amount);
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

void bench_list_json(const struct bench_options *opts, const struct bench_item *items) {
  json_t *list = json_array();

  for(size_t i = 0; items[i].name; i++) {
    json_t *item = json_object();
    json_object_set_new(item, "name", json_string(items[i].name));
    json_object_set_new(item, "info", json_string(items[i].info));
    json_array_append_new(list, item);
  }

  char *out = json_dumps(list, JSON_INDENT(4));
  printf("%s\n", out);

  free(out);
  json_decref(list);
}

void bench_list_text(const struct bench_options *opts, const struct bench_item *items) {
  for(size_t i = 0; items[i].name; i++) {
    printf("%s  %s\n", items[i].name, items[i].info);
  }
}

void bench_list(const struct bench_options *opts, const struct bench_item *items) {
  if(opts->json) {
    bench_list_json(opts, items);
  } else {
    bench_list_text(opts, items);
  }
}

void bench_help(const char *prog) {
  fprintf(
      stderr,
      "passgen_bench\n"
      "Run benchmarks against libpassgen.\n"
      "Usage: %s [OPTIONS] [FILTER]\n\n"
      "OPTIONS\n"
      "  -l, --list         List all available benchmarks.\n"
      "  -d, --duration     The duration to run each test. Default is 50ms. Can be any length,\n"
      "                     accepted units are h, m, s, ms, us, ns.\n"
      "  -t, --threads      How many threads to launch to run benchmarks. Using too many threads\n"
      "                     will slow down the benchmarks.\n"
      "  -v, --version      Show the version of this build.\n"
      "  -h, --help         Show this help text.\n\n"
      "FILTER\n"
      "  A regular expression that is used to filter which benchmarks are to be run.\n",
      prog);
}

void bench_version(void) {
  fprintf(
      stderr,
      "passgen_bench version %s\n",
      "0000");
}

static double parse_time(const char *str) {
  double number;
  char prefix[3];
  int ret = sscanf(str, "%lf%c%c%c", &number, &prefix[0], &prefix[1], &prefix[2]);

  if(ret < 2) {
    return NAN;
  }

  if(ret == 2) {
    switch(prefix[0]) {
      case 'h':
        number *= 60;
      case 'm':
        number *= 60;
      case 's':
        return number;
      default:
        break;
    }
  }

  if(ret == 3 && prefix[1] == 's') {
    switch(prefix[0]) {
      case 'n':
        number /= 1000;
      case 'u':
        number /= 1000;
      case 'm':
        number /= 1000;
        return number;
      default:
        break;
    }
  }

  return NAN;
}

struct bench_options bench_parse_opts(int argc, char *argv[]) {
  struct bench_options opts = {
    .time = 0.05,
    .threads = 1,
    .regex = NULL,
    .json = false,
    .list = false,
    .help = false,
    .version = false,
    .error = false,
  };

  const char *short_opts = "d:t:ljhv";
  static struct option long_opts[] = {
    {"duration", required_argument, NULL, 'd'},
    {"threads", required_argument, NULL, 't'},
    {"list", no_argument, NULL, 'l'},
    {"json", no_argument, NULL, 'j'},
    {"help", no_argument, NULL, 'h'},
    {"version", no_argument, NULL, 'v'},
    {NULL, no_argument, NULL, 0}
  };

  while(true) {
    int opt = getopt_long(argc, argv, short_opts, long_opts, NULL);

    if(opt < 0) break;
    switch(opt) {
      case 0:
        break;
      case 'h':
        opts.help = true;
        break;
      case 't':
        opt = atoi(optarg);
        if(opt < 1) {
        }
        opts.threads = opt;
        break;
      case 'd':
        opts.time = parse_time(optarg);
        if(isnan(opts.time)) {
          fprintf(stderr, "Error parsing time '%s'.\n", optarg);
          opts.error = true;
        }
        break;
      case 'v':
        opts.version = true;
        break;
      case 'l':
        opts.list = true;
        break;
      case 'j':
        opts.json = true;
        break;
      case '?':
      default:
        opts.error = true;
        break;
    }
  }

  // parse optional regex.
  if(optind < argc) {
    opts.regex = calloc(1, sizeof(regex_t));
    assert(opts.regex);
    int ret = regcomp(opts.regex, argv[optind], REG_EXTENDED | REG_ICASE | REG_NOSUB);
    if(ret != 0) {
      fprintf(stderr, "Error parsing regex '%s'.\n", argv[optind]);
      opts.error = true;
      return opts;
    }
  }

  return opts;
}

int main(int argc, char *argv[]) {
  struct bench_options opts = bench_parse_opts(argc, argv);

  if(opts.error) {
    exit(-1);
  }

  if(opts.help) {
    bench_help(argv[0]);
    exit(0);
  }

  if(opts.version) {
    bench_version();
    exit(0);
  }

  if(opts.list) {
    bench_list(&opts, items);
    exit(0);
  }

  bench(&opts, items);

  return 0;
}
