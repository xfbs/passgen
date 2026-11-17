#include "passgen/container/stack.h"
#include "passgen/assert.h"
#include "passgen/config.h"
#include "passgen/util/alloc.h"
#include "passgen/util/clear.h"
#include <stdbool.h>
#include <string.h>

// elements are kept in bins of this many elements. bins other than the first
// are allocated whole, so that what is in them does not move.
static const size_t passgen_stack_bin_size = 32;

// the first bin starts out with room for this many elements, and grows up to
// the size of a bin.
static const size_t passgen_stack_first_init = 4;

// Number of bins which are in use.
static size_t passgen_stack_bins(const passgen_stack *stack) {
    return (stack->len + stack->bin_size - 1) / stack->bin_size;
}

static inline char *passgen_stack_element(
    const passgen_stack *stack,
    size_t bin,
    size_t offset) {
    return (char *) stack->data[bin] + offset * stack->element_size;
}

void passgen_stack_init(passgen_stack *stack, size_t element_size) {
    memset(stack, 0, sizeof(*stack));
    stack->element_size = element_size;
    stack->bin_size = passgen_stack_bin_size;
}

// Releases all of the memory, and leaves the stack empty.
static void passgen_stack_release(passgen_stack *stack) {
    if(!stack->data) {
        return;
    }

    size_t bins = passgen_stack_bins(stack);
    for(size_t i = 1; i < bins; i++) {
        passgen_free(stack->data[i], stack->bin_size * stack->element_size);
    }
    passgen_free(stack->data[0], stack->first_capacity * stack->element_size);
    passgen_free(stack->data, stack->bins_capacity * sizeof(void *));

    stack->data = NULL;
    stack->bins_capacity = 0;
    stack->first_capacity = 0;
    stack->len = 0;
}

void passgen_stack_free(passgen_stack *stack) {
    passgen_stack_release(stack);
    PASSGEN_CLEAR(stack);
}

void passgen_stack_foreach(
    const passgen_stack *stack,
    void (*func)(void *element)) {
    for(size_t i = 0; i < stack->len; i++) {
        func(passgen_stack_get(stack, i));
    }
}

// Makes room in the first bin for one more element.
static bool passgen_stack_grow_first(passgen_stack *stack) {
    // the array of bins
    if(!stack->data) {
        void **data = passgen_alloc(sizeof(void *));
        if(!data) {
            return false;
        }
        data[0] = NULL;
        stack->data = data;
        stack->bins_capacity = 1;
    }

    size_t capacity = stack->first_capacity ? 2 * stack->first_capacity
                                            : passgen_stack_first_init;
    if(capacity > stack->bin_size) {
        capacity = stack->bin_size;
    }

    void *first = passgen_resize(
        stack->data[0],
        stack->first_capacity * stack->element_size,
        capacity * stack->element_size);
    if(!first) {
        // an empty stack does not keep anything allocated.
        if(stack->len == 0) {
            passgen_stack_release(stack);
        }
        return false;
    }

    stack->data[0] = first;
    stack->first_capacity = capacity;
    return true;
}

// Adds a bin, which is the bin with the given index.
static bool passgen_stack_grow_bins(passgen_stack *stack, size_t bin) {
    if(bin == stack->bins_capacity) {
        size_t capacity = 2 * stack->bins_capacity;
        void **data = passgen_resize(
            stack->data,
            stack->bins_capacity * sizeof(void *),
            capacity * sizeof(void *));
        if(!data) {
            return false;
        }
        stack->data = data;
        stack->bins_capacity = capacity;
    }

    void *data = passgen_alloc(stack->bin_size * stack->element_size);
    if(!data) {
        return false;
    }
    stack->data[bin] = data;
    return true;
}

void *passgen_stack_push(passgen_stack *stack, const void *value) {
    size_t bin = stack->len / stack->bin_size;
    size_t offset = stack->len % stack->bin_size;

    if(bin == 0 && stack->len == stack->first_capacity) {
        if(!passgen_stack_grow_first(stack)) {
            return NULL;
        }
    } else if(bin != 0 && offset == 0) {
        if(!passgen_stack_grow_bins(stack, bin)) {
            return NULL;
        }
    }

    char *element = passgen_stack_element(stack, bin, offset);
    stack->len += 1;

    if(value) {
        memcpy(element, value, stack->element_size);
    }

    return element;
}

void *passgen_stack_get(const passgen_stack *stack, size_t pos) {
    if(stack->len <= pos) {
        return NULL;
    }

    return passgen_stack_element(
        stack,
        pos / stack->bin_size,
        pos % stack->bin_size);
}

void *passgen_stack_pop(passgen_stack *stack, void *element) {
    if(!stack->len) {
        return NULL;
    }

    if(element) {
        void *item = passgen_stack_top(stack);
        memcpy(element, item, stack->element_size);
    }

    if(stack->len == 1) {
        // an empty stack does not keep anything allocated.
        passgen_stack_release(stack);
        return element;
    }

    stack->len -= 1;

    // a bin which is empty now is released. the first bin is kept as it is,
    // as making it smaller is not worth it.
    if(stack->len % stack->bin_size == 0) {
        size_t bin = stack->len / stack->bin_size;
        passgen_free(stack->data[bin], stack->bin_size * stack->element_size);
        stack->data[bin] = NULL;
    }

    return element;
}

void *passgen_stack_top(const passgen_stack *stack) {
    return passgen_stack_get(stack, stack->len > 0 ? stack->len - 1 : 0);
}
