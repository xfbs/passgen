/// @file stack.h
/// @author Patrick M. Elsen <pelsen@xfbs.net>
/// @brief Stack data structure
#pragma once
#include <stddef.h>

/// Stack data structure. Implements efficient insertion (push) and removal
/// (pop). Can hold elements of any size, determined by `element_size`.
typedef struct passgen_stack {
    /// Backing data store.
    void **data;

    /// Size of the individual elements that are stored.
    size_t element_size;

    /// Size of the bins that store the actual data.
    size_t bin_size;

    /// Number of items currently in the stack.
    size_t len;
} passgen_stack;

/// Initialize this stack.
void passgen_stack_init(passgen_stack *stack, size_t element_size);

/// Delete the memory for this stack.
void passgen_stack_free(passgen_stack *stack);

/// Execute a function for every item on the stack.
void passgen_stack_foreach(passgen_stack *stack, void (*func)(void *element));

/// Push a new value onto this stack. If a non-NULL pointer is passed, it is
/// copied to the new value. A pointer to the value on the stack is returned.
void *passgen_stack_push(passgen_stack *stack, void *value);

/// Get the nth element on the stack. If the position is invalid, returns NULL.
void *passgen_stack_get(passgen_stack *stack, size_t pos);

/// Pop the largest element off the stack. If a non-NULL pointer is passed,
/// the element's memory is copied into that value.
void *passgen_stack_pop(passgen_stack *stack, void *element);

/// Returns the topmost element of the stack, or NULL if the stack is empty.
void *passgen_stack_top(passgen_stack *stack);
