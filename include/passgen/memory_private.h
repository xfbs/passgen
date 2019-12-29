#pragma once
#include "passgen/memory.h"
#include "passgen/debug.h"

struct passgen_mem_accounting_list {
    void *pointer;
    size_t count;
    size_t size;
    bool freed;
#ifdef PASSGEN_DEBUG
    void *alloc_bt[255];
    size_t alloc_frames;
    void *free_bt[255];
    size_t free_frames;
#endif
    struct passgen_mem_accounting_list *next;
};

