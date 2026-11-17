/// @file stack.h
/// @author Patrick M. Elsen <pelsen@xfbs.net>
/// @brief Stack data structure
#pragma once
#include <stddef.h>

/// Stack data structure. Implements efficient insertion (push) and removal
/// (pop). Can hold elements of any size, determined by `element_size`.
typedef struct {
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
///
/// @memberof passgen_stack
/// @param stack Stack to initialize
/// @param element_size Size of elements on this stack
void passgen_stack_init(passgen_stack *stack, size_t element_size);

/// Free the memory for this stack.
///
/// @memberof passgen_stack
/// @param stack Stack to free
void passgen_stack_free(passgen_stack *stack);

/// Execute a function for every item on the stack.
///
/// @memberof passgen_stack
/// @param stack Stack to iterate over
/// @param func Function to call for every item in the stack
void passgen_stack_foreach(const passgen_stack *stack, void (*func)(void *element));

/// Push a new value onto this stack. If a non-NULL pointer is passed, it is
/// copied to the new value. A pointer to the value on the stack is returned.
///
/// @memberof passgen_stack
/// @param stack Stack to push into
/// @param value Pointer to value to insert. If NULL, new space is created on the stack but it is not initialized.
/// @returns Pointer to the new item on the stack.
void *passgen_stack_push(passgen_stack *stack, void *value);

/// Get the nth element on the stack. If the position is invalid, returns NULL.
///
/// @memberof passgen_stack
/// @param stack Stack to get element from
/// @param pos Position of element to get
/// @returns Pointer to the element if it exists, else NULL.
void *passgen_stack_get(const passgen_stack *stack, size_t pos);

/// Pop the largest element off the stack. If a non-NULL pointer is passed,
/// the element's memory is copied into that value.
///
/// @memberof passgen_stack
/// @param stack Stack to pop element from
/// @param element Element to copy data into if not NULL
void *passgen_stack_pop(passgen_stack *stack, void *element);

/// Returns the topmost element of the stack, or NULL if the stack is empty.
///
/// @memberof passgen_stack
/// @param stack Stack to lookup top element in
/// @returns Pointer to topmost element on the stack.
void *passgen_stack_top(const passgen_stack *stack);
