#pragma once
#include <stddef.h>

#define bench_declare(_name_)                        \
     extern const char bench_##_name_##_name[];      \
     extern const char bench_##_name_##_info[];      \
     void *bench_##_name_##_setup(const char *data); \
     void bench_##_name_##_func(void *state);        \
     void bench_##_name_##_free(void *state);

#define bench_define(_name_)                                              \
     {                                                                    \
          .name = bench_##_name_##_name, .info = bench_##_name_##_info,   \
          .setup = bench_##_name_##_setup, .func = bench_##_name_##_func, \
          .free = bench_##_name_##_free                                   \
     }

#define bench_new(_name_, _info_)                  \
     const char bench_##_name_##_name[] = #_name_; \
     const char bench_##_name_##_info[] = _info_

typedef void *bench_setup_func(const char *params);
typedef void bench_func(void *state);
typedef void bench_free_func(void *state);

struct bench_item {
     const char *name;
     const char *info;
     bench_setup_func *setup;
     bench_func *func;
     bench_free_func *free;
};

struct bench_run {
     struct bench_item *item;
     size_t batch;
     const char *data;
};

extern struct bench_item items[];
extern struct bench_run bench_runs[];
