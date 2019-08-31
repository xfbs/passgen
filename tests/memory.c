#include "passgen/memory.h"
#include "tests/tests.h"
#include <string.h>

test_result test_memory_stdlib(void) {
    passgen_mem_t mem = passgen_mem_stdlib();

    assert(mem.malloc == NULL);
    assert(mem.calloc == NULL);
    assert(mem.realloc == NULL);
    assert(mem.free == NULL);
    assert(mem.state == NULL);

    return test_ok;
}

test_result test_memory_stdlib_alloc(void) {
    passgen_mem_t mem = passgen_mem_stdlib();

    char *m_data = passgen_malloc(&mem, 32);
    assert(m_data);
    memset(m_data, 0, 32);

    char *c_data = passgen_calloc(&mem, 2, 16);
    assert(c_data);

    char *r_data = passgen_realloc(&mem, NULL, 32);
    assert(r_data);
    memset(r_data, 0, 32);

    for(size_t i = 0; i < 32; i++) {
        assert(m_data[i] == 0);
        assert(c_data[i] == 0);
        assert(r_data[i] == 0);
    }

    m_data = passgen_realloc(&mem, m_data, 1024);
    assert(m_data);
    memset(m_data, 0, 1024);

    r_data = passgen_realloc(&mem, r_data, 1024);
    assert(r_data);
    memset(r_data, 0, 1024);

    c_data = passgen_realloc(&mem, c_data, 1024);
    assert(c_data);
    memset(c_data, 0, 1024);

    for(size_t i = 0; i < 1024; i++) {
        assert(m_data[i] == 0);
        assert(c_data[i] == 0);
        assert(r_data[i] == 0);
    }

    passgen_free(&mem, m_data);
    passgen_free(&mem, c_data);
    passgen_free(&mem, r_data);

    return test_ok;
}

test_result test_memory_limited(void) {
    passgen_mem_t std_mem = passgen_mem_stdlib();
    passgen_mem_limits_t limits = passgen_mem_limits(&std_mem, 32, 8);
    passgen_mem_t mem = passgen_mem_limited(&limits);

    assert(mem.malloc != NULL);
    assert(mem.calloc != NULL);
    assert(mem.realloc != NULL);
    assert(mem.free != NULL);
    assert(mem.state == &limits);

    return test_ok;
}

test_result test_memory_limited_malloc(void) {
    passgen_mem_t std_mem = passgen_mem_stdlib();
    passgen_mem_limits_t limits = passgen_mem_limits(&std_mem, 32, 0);
    passgen_mem_t mem = passgen_mem_limited(&limits);
    void *data1, *data2, *data3;

    data1 = passgen_malloc(&mem, 32);
    assert(data1);
    memset(data1, 0, 32);

    data2 = passgen_malloc(&mem, 32);
    assert(!data2);

    passgen_free(&mem, data1);

    limits = passgen_mem_limits(&std_mem, 0, 2);

    data1 = passgen_malloc(&mem, 32);
    assert(data1);
    memset(data1, 0, 32);

    data2 = passgen_malloc(&mem, 32);
    assert(data2);
    memset(data2, 0, 32);

    data3 = passgen_malloc(&mem, 32);
    assert(!data3);

    passgen_free(&mem, data1);
    passgen_free(&mem, data2);
    passgen_free(&mem, data3);

    return test_ok;
}
