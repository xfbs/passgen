#include <criterion/criterion.h>
#include <passgen/context.h>

Test(context, init) {
    passgen_context context;
    passgen_context_init(&context);
    cr_assert_eq(context.malloc, NULL);
    cr_assert_eq(context.calloc, NULL);
    cr_assert_eq(context.realloc, NULL);
    cr_assert_eq(context.free, NULL);
    cr_assert_eq(context.alloc_state, NULL);
}
