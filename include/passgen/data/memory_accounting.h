#pragma once

struct passgen_mem_accounting_list;
struct passgen_mem_accounting {
  struct passgen_mem *mem;
  struct passgen_mem_accounting_list *list;
};

typedef struct passgen_mem_accounting passgen_mem_accounting_t;
