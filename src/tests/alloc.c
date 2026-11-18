#include "passgen/util/alloc.h"
#include "testalloc.h"
#include "tests.h"
#include <stdlib.h>
#include <string.h>

// every allocation has a header which remembers its size. this is as large
// as the largest alignment, so that what follows it is aligned as well.
typedef union {
    size_t size;
    long double align_ld;
    long long align_ll;
    void *align_ptr;
} header;

// Whether the next request fails.
static bool test_fails(test_allocator *allocator) {
    size_t request = allocator->requests++;
    return request >= allocator->fail_at &&
           request - allocator->fail_at < allocator->fail_count;
}

static void *test_alloc(void *state, size_t size) {
    test_allocator *allocator = state;
    if(test_fails(allocator)) {
        return NULL;
    }

    header *block = malloc(sizeof(header) + size);
    if(!block) {
        return NULL;
    }
    block->size = size;
    allocator->live += 1;
    allocator->live_bytes += size;
    return block + 1;
}

static header *test_header(test_allocator *allocator, void *ptr, size_t size) {
    header *block = (header *) ptr - 1;
    if(block->size != size) {
        allocator->mismatches += 1;
    }
    return block;
}

static void *
test_resize(void *state, void *ptr, size_t old_size, size_t new_size) {
    test_allocator *allocator = state;
    header *block = test_header(allocator, ptr, old_size);
    if(test_fails(allocator)) {
        return NULL;
    }

    // always moves, so that anything which keeps using the old pointer is
    // found by the sanitizers.
    header *moved = malloc(sizeof(header) + new_size);
    if(!moved) {
        return NULL;
    }
    size_t keep = block->size < new_size ? block->size : new_size;
    memcpy(moved + 1, block + 1, keep);
    moved->size = new_size;
    allocator->live_bytes += new_size;
    allocator->live_bytes -= block->size;
    free(block);
    return moved + 1;
}

static void test_free(void *state, void *ptr, size_t size) {
    test_allocator *allocator = state;
    header *block = test_header(allocator, ptr, size);
    allocator->live -= 1;
    allocator->live_bytes -= block->size;
    free(block);
}

void test_allocator_install(test_allocator *allocator, size_t fail_at) {
    memset(allocator, 0, sizeof(*allocator));
    allocator->fail_at = fail_at;
    allocator->fail_count = TEST_ALLOCATOR_NEVER;
    passgen_allocator config = {
        .alloc = test_alloc,
        .resize = test_resize,
        .free = test_free,
        .state = allocator,
    };
    passgen_allocator_set(&config);
}

void test_allocator_remove(void) {
    passgen_allocator_set(NULL);
}

bool test_allocator_clean(const test_allocator *allocator) {
    return allocator->live == 0 && allocator->live_bytes == 0 &&
           allocator->mismatches == 0;
}

/// Without being replaced, the allocator is malloc().
test_result test_alloc_default(void) {
    char *data = passgen_alloc(16);
    assert(data);
    memset(data, 'a', 16);

    data = passgen_resize(data, 16, 4096);
    assert(data);
    assert(data[15] == 'a');
    passgen_free(data, 4096);

    // nothing is allocated as well, and freeing NULL does nothing.
    data = passgen_alloc(0);
    assert(data);
    passgen_free(data, 0);
    passgen_free(NULL, 10);

    // resizing NULL allocates.
    data = passgen_resize(NULL, 0, 8);
    assert(data);
    passgen_free(data, 8);

    uint8_t *zeroed = passgen_alloc_zeroed(64);
    assert(zeroed);
    for(size_t i = 0; i < 64; i++) {
        assert(zeroed[i] == 0);
    }
    passgen_free(zeroed, 64);

    return test_ok;
}

/// A replaced allocator is used for everything, until it is put back.
test_result test_alloc_replace(void) {
    test_allocator allocator;
    test_allocator_install(&allocator, TEST_ALLOCATOR_NEVER);

    void *first = passgen_alloc(10);
    assert(first);
    void *second = passgen_alloc_zeroed(20);
    assert(second);
    assert(allocator.requests == 2);
    assert(allocator.live == 2);
    assert(allocator.live_bytes == 30);

    first = passgen_resize(first, 10, 100);
    assert(first);
    assert(allocator.live == 2);
    assert(allocator.live_bytes == 120);

    passgen_free(first, 100);
    passgen_free(second, 20);
    assert(test_allocator_clean(&allocator));

    // put back: the test allocator does not see this.
    test_allocator_remove();
    void *data = passgen_alloc(10);
    assert(data);
    passgen_free(data, 10);
    assert(allocator.requests == 3);

    return test_ok;
}

/// The test allocator fails when it is asked to, and a resize which fails
/// keeps the allocation.
test_result test_alloc_fail(void) {
    test_allocator allocator;
    test_allocator_install(&allocator, 2);

    char *first = passgen_alloc(4);
    assert(first);
    memcpy(first, "abc", 4);
    char *second = passgen_alloc(4);
    assert(second);
    assert(!passgen_alloc(4));
    assert(!passgen_alloc_zeroed(4));
    assert(!passgen_resize(first, 4, 8));
    assert(0 == strcmp(first, "abc"));

    passgen_free(first, 4);
    passgen_free(second, 4);
    assert(test_allocator_clean(&allocator));
    test_allocator_remove();

    // failing right away.
    test_allocator_install(&allocator, 0);
    assert(!passgen_alloc(1));
    assert(!passgen_alloc_string("abc"));
    assert(test_allocator_clean(&allocator));
    test_allocator_remove();

    // failing once.
    test_allocator_install(&allocator, 1);
    allocator.fail_count = 1;
    first = passgen_alloc(1);
    assert(first);
    assert(!passgen_alloc(1));
    second = passgen_alloc(1);
    assert(second);
    passgen_free(first, 1);
    passgen_free(second, 1);
    assert(test_allocator_clean(&allocator));
    test_allocator_remove();

    return test_ok;
}

/// Releasing with the wrong size is noticed by the test allocator, which is
/// what the tests rely on to check the sizes.
test_result test_alloc_size_checked(void) {
    test_allocator allocator;
    test_allocator_install(&allocator, TEST_ALLOCATOR_NEVER);

    void *data = passgen_alloc(10);
    assert(data);
    passgen_free(data, 11);
    assert(allocator.mismatches == 1);
    assert(allocator.live == 0);
    assert(!test_allocator_clean(&allocator));
    test_allocator_remove();

    return test_ok;
}

/// Strings are copied, and released by their length.
test_result test_alloc_string(void) {
    test_allocator allocator;
    test_allocator_install(&allocator, TEST_ALLOCATOR_NEVER);

    char *copy = passgen_alloc_string("hello");
    assert(copy);
    assert_streq(copy, "hello");
    assert(allocator.live_bytes == 6);
    passgen_free_string(copy);
    passgen_free_string(NULL);

    copy = passgen_alloc_string("");
    assert(copy);
    assert_streq(copy, "");
    passgen_free_string(copy);

    assert(test_allocator_clean(&allocator));
    test_allocator_remove();

    return test_ok;
}
