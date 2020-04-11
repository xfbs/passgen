#pragma once
#include "passgen/container/stack.h"
#include "passgen/memory.h"

#define BINS_INITIAL        4
#define BINS_MULTIPLIER     2
#define BIN_SIZE            1024
#define ITEMS_PER_BIN(size) (BIN_SIZE / size)

#define PASSGEN_STACK_IMPLEMENT(data_type, name)                             \
  void name##_stack_init(struct name##_stack *stack, passgen_mem_t *mem) {   \
    (void)mem;                                                               \
                                                                             \
    stack->data = NULL;                                                      \
    stack->len = 0;                                                          \
    stack->bins = 0;                                                         \
  }                                                                          \
                                                                             \
  data_type *name##_stack_push(                                              \
      struct name##_stack *stack,                                            \
      passgen_mem_t *mem) {                                                  \
    if(!stack->bins) {                                                       \
      stack->data = passgen_malloc(mem, BINS_INITIAL * sizeof(data_type *)); \
      if(!stack->data) {                                                     \
        return NULL;                                                         \
      } else {                                                               \
        stack->bins = BINS_INITIAL;                                          \
      }                                                                      \
    }                                                                        \
                                                                             \
    if(stack->len == (stack->bins * ITEMS_PER_BIN(sizeof(data_type)))) {     \
      size_t new_bins = stack->bins * BINS_MULTIPLIER;                       \
      data_type **new_data =                                                 \
          passgen_realloc(mem, stack->data, new_bins * sizeof(data_type *)); \
      if(!new_data) {                                                        \
        return NULL;                                                         \
      } else {                                                               \
        stack->data = new_data;                                              \
        stack->bins = new_bins;                                              \
      }                                                                      \
    }                                                                        \
                                                                             \
    size_t bin = stack->len / ITEMS_PER_BIN(sizeof(data_type));              \
    size_t offset = stack->len % ITEMS_PER_BIN(sizeof(data_type));           \
                                                                             \
    if(offset == 0) {                                                        \
      stack->data[bin] = passgen_malloc(                                     \
          mem,                                                               \
          ITEMS_PER_BIN(sizeof(data_type)) * sizeof(data_type));             \
      if(!stack->data[bin]) {                                                \
        return NULL;                                                         \
      }                                                                      \
    }                                                                        \
                                                                             \
    stack->len += 1;                                                         \
    return &stack->data[bin][offset];                                        \
  }                                                                          \
                                                                             \
  data_type *name##_stack_get(struct name##_stack *stack, size_t pos) {      \
    size_t bin = pos / ITEMS_PER_BIN(sizeof(data_type));                     \
    size_t offset = pos % ITEMS_PER_BIN(sizeof(data_type));                  \
    return &stack->data[bin][offset];                                        \
  }                                                                          \
                                                                             \
  void name##_stack_free(struct name##_stack *stack, passgen_mem_t *mem) {   \
    size_t bins = (stack->len + ITEMS_PER_BIN(sizeof(data_type)) - 1) /      \
                  ITEMS_PER_BIN(sizeof(data_type));                          \
                                                                             \
    for(size_t i = 0; i < bins; i++) {                                       \
      passgen_free(mem, stack->data[i]);                                     \
    }                                                                        \
                                                                             \
    if(bins) {                                                               \
      passgen_free(mem, stack->data);                                        \
    }                                                                        \
  }                                                                          \
                                                                             \
  void name##_stack_pop(struct name##_stack *stack, passgen_mem_t *mem) {    \
    size_t bin = stack->len / ITEMS_PER_BIN(sizeof(data_type));              \
    size_t offset = stack->len % ITEMS_PER_BIN(sizeof(data_type));           \
                                                                             \
    if(offset == 1) {                                                        \
      passgen_free(mem, stack->data[bin]);                                   \
      stack->data[bin] = NULL;                                               \
                                                                             \
      if(bin == 0) {                                                         \
        passgen_free(mem, stack->data);                                      \
        stack->data = NULL;                                                  \
        stack->bins = 0;                                                     \
      }                                                                      \
    }                                                                        \
                                                                             \
    stack->len -= 1;                                                         \
  } \
  \
  data_type *name##_stack_top(struct name##_stack *stack) { \
    if(stack->len) { \
      return name##_stack_get(stack, stack->len - 1); \
    } else { \
      return NULL; \
    } \
  }
