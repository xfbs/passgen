#include "passgen/stack.h"
#include "passgen/assert.h"
#include <string.h>
#include <stdlib.h>

static const size_t passgen_stack_bin_size = 32;
static const size_t passgen_stack_cap_init = 4;

static size_t passgen_stack_bins(const passgen_stack *stack) {
    return (stack->len + stack->bin_size - 1) / stack->bin_size;
}

static size_t passgen_stack_bin(const passgen_stack *stack, size_t pos) {
    return pos / stack->bin_size;
}

static size_t passgen_stack_bin_count(const passgen_stack *stack, size_t len) {
    (void) stack;
    size_t power = 1;
    while(power < len) {
            power *= 2;
    }
    return power;
}

// Initialize this stack.
void passgen_stack_init(passgen_stack *stack, size_t element_size) {
    memset(stack, 0, sizeof(*stack));
    stack->element_size = element_size;
    stack->bin_size = passgen_stack_bin_size;
}

// Delete the memory for this stack.
void passgen_stack_free(passgen_stack *stack) {
    size_t bins = passgen_stack_bins(stack);
    for(size_t i = 0; i < bins; i++) {
        free(stack->data[i]);
    }

    if(stack->len) {
        free(stack->data);
    }

    memset(stack, 0, sizeof(*stack));
}

// Execute a function for every item on the stack.
void passgen_stack_foreach(passgen_stack *stack, void (*func)(void *element)) {
    size_t full_bins = stack->len / stack->bin_size;
    for(size_t i = 0; i < full_bins; i++) {
        void *bin = stack->data[i];
        for(size_t j = 0; j < stack->bin_size; j++) {
            func(bin + j * stack->element_size);
        }
    }
    size_t last_bin_items = stack->len % stack->bin_size;
    for(size_t i = 0; i < last_bin_items; i++) {
        void *bin = stack->data[full_bins];
        func(bin + i * stack->element_size);
    }
}

// Push a new value onto this stack. If a non-NULL pointer is passed, it is
// copied to the new value. A pointer to the value on the stack is returned.
void *passgen_stack_push(passgen_stack *stack, void *value) {
    size_t new_bin = passgen_stack_bin(stack, stack->len);

    // initially allocate data
    if(!new_bin) {
        if(!stack->len) {
            stack->data = calloc(1, sizeof(void *));
            stack->data[0] = calloc(passgen_stack_cap_init, stack->element_size);
        }
        if(passgen_stack_bin_count(stack, stack->len) == stack->len) {
            size_t new_size = passgen_stack_bin_count(stack, stack->len + 1);
            stack->data[0] = realloc(stack->data[0], new_size * stack->element_size);
        }
    } else {
        size_t max_bin = passgen_stack_bin(stack, stack->len - 1);

        // do we need to increase the bins?
        if(new_bin != max_bin) {
            size_t current_bins = passgen_stack_bin_count(stack, new_bin);
            size_t needed_bins = passgen_stack_bin_count(stack, new_bin + 1);
            if(current_bins != needed_bins) {
                void **new_data = realloc(stack->data, needed_bins * sizeof(void *));
                passgen_assert(new_data);
                stack->data = new_data;
            }
            stack->data[new_bin] = calloc(stack->bin_size, stack->element_size);
        }
    }

    size_t offset = stack->len % stack->bin_size;
    void *data = stack->data[new_bin] + offset * stack->element_size;

    stack->len += 1;
    if(value) {
        memcpy(data, value, stack->element_size);
    }

    return data;
}

// Get the nth element on the stack. If the position is invalid, returns NULL.
void *passgen_stack_get(passgen_stack *stack, size_t pos) {
    if(stack->len <= pos) {
        return NULL;
    }

    size_t bin = pos / stack->bin_size;
    size_t offset = pos % stack->bin_size;
    return stack->data[bin] + offset * stack->element_size;
}

// Pop the largest element off the stack. If a non-NULL pointer is passed,
// the element's memory is copied into that value, and the passed pointer
// is returned. If the stack is empty, a NULL pointer is returned.
void *passgen_stack_pop(passgen_stack *stack, void *element) {
    if(!stack->len) {
        return NULL;
    }

    if(element) {
        void *item = passgen_stack_top(stack);
        memcpy(element, item, stack->element_size);
    }

    if(stack->len == 1) {
        free(stack->data[0]);
        stack->data[0] = NULL;
        free(stack->data);
        stack->data = NULL;
    } else if(stack->len <= stack->bin_size) {
        size_t entry_size = passgen_stack_bin_count(stack, stack->len);
        size_t new_size = passgen_stack_bin_count(stack, stack->len - 1);
        if(entry_size != new_size) {
            stack->data[0] = realloc(stack->data[0], new_size * stack->element_size);
        }
    } else {
        size_t max_bin = passgen_stack_bin(stack, stack->len - 1);
        size_t new_bin = passgen_stack_bin(stack, stack->len - 2);
        if(max_bin != new_bin) {
            free(stack->data[max_bin]);
            stack->data[max_bin] = NULL;
        }
    }

    stack->len -= 1;

    return element;
}

// Returns the topmost element of the stack, or NULL if the stack is empty.
void *passgen_stack_top(passgen_stack *stack) {
    return passgen_stack_get(stack, stack->len > 0 ? stack->len - 1 : 0);
}
