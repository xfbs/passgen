#include "passgen/memory.h"
#include "passgen/memory_private.h"
#include "tests.h"
#include <string.h>

test_result test_memory_stdlib(void) {
     char *m_data = passgen_malloc(NULL, 32);
     assert(m_data);
     memset(m_data, 0, 32);

     char *c_data = passgen_calloc(NULL, 2, 16);
     assert(c_data);

     char *r_data = passgen_realloc(NULL, NULL, 32);
     assert(r_data);
     memset(r_data, 0, 32);

     for(size_t i = 0; i < 32; i++) {
          assert(m_data[i] == 0);
          assert(c_data[i] == 0);
          assert(r_data[i] == 0);
     }

     m_data = passgen_realloc(NULL, m_data, 1024);
     assert(m_data);
     memset(m_data, 0, 1024);

     r_data = passgen_realloc(NULL, r_data, 1024);
     assert(r_data);
     memset(r_data, 0, 1024);

     c_data = passgen_realloc(NULL, c_data, 1024);
     assert(c_data);
     memset(c_data, 0, 1024);

     for(size_t i = 0; i < 1024; i++) {
          assert(m_data[i] == 0);
          assert(c_data[i] == 0);
          assert(r_data[i] == 0);
     }

     passgen_free(NULL, m_data);
     passgen_free(NULL, c_data);
     passgen_free(NULL, r_data);

     return test_ok;
}

test_result test_memory_limited(void) {
     passgen_mem_limits_t limits = passgen_mem_limits(NULL, 32, 8);
     passgen_mem_t mem = passgen_mem_limited(&limits);

     assert(mem.malloc != NULL);
     assert(mem.calloc != NULL);
     assert(mem.realloc != NULL);
     assert(mem.free != NULL);
     assert(mem.state == &limits);

     return test_ok;
}

test_result test_memory_limited_malloc(void) {
     passgen_mem_limits_t limits = passgen_mem_limits(NULL, 32, 0);
     passgen_mem_t mem = passgen_mem_limited(&limits);
     void *data1, *data2, *data3;

     data1 = passgen_malloc(&mem, 32);
     assert(data1);
     memset(data1, 0, 32);

     data2 = passgen_malloc(&mem, 32);
     assert(!data2);

     passgen_free(&mem, data1);

     limits = passgen_mem_limits(NULL, 0, 2);

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

test_result test_memory_accounting(void) {
     passgen_mem_accounting_t acc = passgen_mem_accounting_new(NULL);
     passgen_mem_t mem = passgen_mem_accounting(&acc);
     struct passgen_mem_accounting_list *list;

     void *data, *data1, *data2, *data3;

     data = passgen_malloc(&mem, 1024);
     assert(data);

     list = acc.list;
     assert(list->pointer == data);
     assert(list->freed == false);
     assert(list->count == 1);
     assert(list->size == 1024);
     assert(list->next == NULL);

     data1 = passgen_calloc(&mem, 24, 15);
     assert(data1);

     list = acc.list;
     assert(list->pointer == data1);
     assert(list->freed == false);
     assert(list->count == 24);
     assert(list->size == 15);
     assert(list->next != NULL);

     passgen_free(&mem, data);

     list = acc.list->next;
     assert(list->pointer == data);
     assert(list->freed == true);
     assert(list->count == 1);
     assert(list->size == 1024);
     assert(list->next == NULL);

     data2 = passgen_realloc(&mem, data1, 7000);
     assert(data2);

     // this assertion might not hold true.
     assert(data2 != data1);

     list = acc.list;
     assert(list->pointer == data2);
     assert(list->freed == false);
     assert(list->count == 1);
     assert(list->size == 7000);
     assert(list->next != NULL);

     list = acc.list->next;
     assert(list->pointer == data1);
     assert(list->freed == true);
     assert(list->count == 24);
     assert(list->size == 15);
     assert(list->next != NULL);

     passgen_free(&mem, data2);

     list = acc.list;
     assert(list->pointer == data2);
     assert(list->freed == true);
     assert(list->count == 1);
     assert(list->size == 7000);
     assert(list->next != NULL);

     assert(passgen_mem_accounting_check(&acc));

     data3 = passgen_realloc(&mem, NULL, 4000);
     list = acc.list;
     assert(list->pointer == data3);
     assert(list->freed == false);
     assert(list->count == 1);
     assert(list->size == 4000);
     assert(list->next != NULL);

     assert(!passgen_mem_accounting_check(&acc));

     passgen_free(&mem, data3);
     assert(passgen_mem_accounting_check(&acc));

     passgen_mem_accounting_cleanup(&acc);

     return test_ok;
}
