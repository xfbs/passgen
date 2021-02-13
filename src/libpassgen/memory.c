#include <passgen/config.h>
#include <passgen/data/memory.h>
#include <passgen/memory.h>
#include <passgen/memory_private.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef PASSGEN_DEBUG
#    include <execinfo.h>
#endif

enum passgen_mem_accounting_error {
    // pointer being freed was already freed
    PASSGEN_MEM_ACCOUNTING_DOUBLEFREE,
    // pointer being freed is unknown
    PASSGEN_MEM_ACCOUNTING_FREEBADPTR,
    // pointer being reallocated is unknown
    PASSGEN_MEM_ACCOUNTING_REALLOCBADPTR,
    // pointer being reallocated was freed
    PASSGEN_MEM_ACCOUNTING_REALLOCFREED,
};

void *passgen_malloc(struct passgen_mem *mem, size_t size) {
    if(!mem) {
        return malloc(size);
    } else {
        return mem->malloc(mem->state, size);
    }
}

void passgen_free(struct passgen_mem *mem, void *ptr) {
    if(!mem) {
        free(ptr);
    } else {
        mem->free(mem->state, ptr);
    }
}

void *passgen_realloc(struct passgen_mem *mem, void *ptr, size_t size) {
    if(!mem) {
        return realloc(ptr, size);
    } else {
        return mem->realloc(mem->state, ptr, size);
    }
}

void *passgen_calloc(struct passgen_mem *mem, size_t size, size_t count) {
    if(!mem) {
        return calloc(size, count);
    } else {
        return mem->calloc(mem->state, size, count);
    }
}

static void *passgen_malloc_limited(void *state, size_t size) {
    struct passgen_mem_limits *limits = state;

    if(limits->allocs_limit && limits->allocs_limit == limits->allocs) {
        return NULL;
    }

    if(limits->size_limit && limits->size_limit < (limits->size + size)) {
        return NULL;
    }

    limits->allocs += 1;
    limits->size += size;

    return passgen_malloc(limits->mem, size);
}

static void *passgen_calloc_limited(void *state, size_t count, size_t size) {
    struct passgen_mem_limits *limits = state;

    if(limits->allocs_limit && limits->allocs_limit == limits->allocs) {
        return NULL;
    }

    if(limits->size_limit &&
       limits->size_limit < (limits->size + count * size)) {
        return NULL;
    }

    limits->allocs += 1;
    limits->size += count * size;

    return passgen_calloc(limits->mem, count, size);
}

static void *passgen_realloc_limited(void *state, void *ptr, size_t size) {
    struct passgen_mem_limits *limits = state;

    if(limits->allocs_limit && limits->allocs_limit == limits->allocs) {
        return NULL;
    }

    if(limits->size_limit && limits->size_limit < (limits->size + size)) {
        return NULL;
    }

    limits->allocs += 1;
    limits->size += size;

    return passgen_realloc(limits->mem, ptr, size);
}

static void passgen_free_limited(void *state, void *ptr) {
    struct passgen_mem_limits *limits = state;

    passgen_free(limits->mem, ptr);
}

void passgen_mem_accounting_error(
    enum passgen_mem_accounting_error error,
    void *data) {
    struct passgen_mem_accounting_list *node = NULL;
    const char *errstr;
    void *pointer = data;

    switch(error) {
        case PASSGEN_MEM_ACCOUNTING_DOUBLEFREE:
            errstr = "pointer being freed was already freed.";
            node = data;
            pointer = node->pointer;
            break;
        case PASSGEN_MEM_ACCOUNTING_FREEBADPTR:
            errstr = "pointer being freed is unknown.";
            break;
        case PASSGEN_MEM_ACCOUNTING_REALLOCBADPTR:
            errstr = "pointer being reallocated is unknown.";
            break;
        case PASSGEN_MEM_ACCOUNTING_REALLOCFREED:
            errstr = "pointer being reallocated was freed.";
            node = data;
            pointer = node->pointer;
            break;
    }

    fprintf(stderr, "Fatal error: %s\n", errstr);
    fprintf(stderr, "Pointer %p\n", pointer);

#if defined(PASSGEN_DEBUG) && defined(PASSGEN_BACKTRACE)
    // print backtrace
    fprintf(stderr, "\nBacktrace:\n");
    void *callstack[255];
    int frames = backtrace(callstack, sizeof(callstack));
    char **strs = backtrace_symbols(callstack, frames);
    for(int i = 0; i < frames; ++i) {
        printf("%s\n", strs[i]);
    }
    free(strs);

    if(node) {
        // print allocation backtrace
        fprintf(stderr, "\nBacktrace (allocation):\n");
        char **symbols = backtrace_symbols(node->alloc_bt, node->alloc_frames);
        for(size_t i = 0; i < node->alloc_frames; i++) {
            printf("%s\n", symbols[i]);
        }
        free(symbols);

        fprintf(stderr, "\nBacktrace (initial free):\n");
        symbols = backtrace_symbols(node->free_bt, node->free_frames);
        for(size_t i = 0; i < node->free_frames; i++) {
            printf("%s\n", symbols[i]);
        }
        free(symbols);
    }
#endif

    exit(EXIT_FAILURE);
}

static void *passgen_malloc_accounting(void *state, size_t size) {
    struct passgen_mem_accounting *acc = state;

    // allocate memory for list node
    struct passgen_mem_accounting_list *node =
        passgen_malloc(acc->mem, sizeof(struct passgen_mem_accounting_list));

    // make sure this node exists
    if(!node) {
        return NULL;
    }

    // allocate memory
    node->size = size;
    node->pointer = passgen_malloc(acc->mem, size);
    node->freed = false;
    node->count = 1;

    // make sure we could allocate memory for this
    if(!node->pointer) {
        passgen_free(acc->mem, node);
        return NULL;
    }

    // keep track of where in the code this allocation happened.
#if defined(PASSGEN_DEBUG) && defined(PASSGEN_BACKTRACE)
    node->alloc_frames = backtrace(node->alloc_bt, sizeof(node->alloc_bt));
#endif

    // assert this node at the head of the list
    node->next = acc->list;
    acc->list = node;

    return node->pointer;
}

static void *passgen_calloc_accounting(void *state, size_t count, size_t size) {
    struct passgen_mem_accounting *acc = state;

    // allocate memory for list node
    struct passgen_mem_accounting_list *node =
        passgen_malloc(acc->mem, sizeof(struct passgen_mem_accounting_list));

    // make sure this node exists
    if(!node) {
        return NULL;
    }

    // allocate memory
    node->size = size;
    node->count = count;
    node->pointer = passgen_calloc(acc->mem, count, size);
    node->freed = false;

    // make sure we could allocate memory for this
    if(!node->pointer) {
        passgen_free(acc->mem, node);
        return NULL;
    }

    // keep track of where in the code this allocation happened.
#if defined(PASSGEN_DEBUG) && defined(PASSGEN_BACKTRACE)
    node->alloc_frames = backtrace(node->alloc_bt, sizeof(node->alloc_bt));
#endif

    // assert this node at the head of the list
    node->next = acc->list;
    acc->list = node;

    return node->pointer;
}

static void *passgen_realloc_accounting(void *state, void *ptr, size_t size) {
    struct passgen_mem_accounting *acc = state;
    struct passgen_mem_accounting_list *ptr_node = NULL;

    if(ptr) {
        // find previous allocation
        for(ptr_node = acc->list; ptr_node; ptr_node = ptr_node->next) {
            if(ptr_node->pointer == ptr) {
                break;
            }
        }

        if(!ptr_node) {
            passgen_mem_accounting_error(
                PASSGEN_MEM_ACCOUNTING_REALLOCBADPTR,
                ptr);
        }

        if(ptr_node->freed) {
            passgen_mem_accounting_error(
                PASSGEN_MEM_ACCOUNTING_REALLOCFREED,
                ptr_node);
        }
    }

    // allocate memory for list node
    struct passgen_mem_accounting_list *node =
        passgen_malloc(acc->mem, sizeof(struct passgen_mem_accounting_list));

    // make sure this node exists
    if(!node) {
        return NULL;
    }

    // allocate memory
    node->size = size;
    node->count = 1;
    node->pointer = passgen_realloc(acc->mem, ptr, size);
    node->freed = false;

    // make sure we could allocate memory for this
    if(!node->pointer) {
        passgen_free(acc->mem, node);
        return NULL;
    }

    // keep track of where in the code this allocation happened.
#if defined(PASSGEN_DEBUG) && defined(PASSGEN_BACKTRACE)
    node->alloc_frames = backtrace(node->alloc_bt, sizeof(node->alloc_bt));
#endif

    // assert this node at the head of the list
    node->next = acc->list;
    acc->list = node;

    if(ptr_node) {
        ptr_node->freed = true;

#if defined(PASSGEN_DEBUG) && defined(PASSGEN_BACKTRACE)
        ptr_node->free_frames =
            backtrace(ptr_node->free_bt, sizeof(ptr_node->free_bt));
#endif
    }

    return node->pointer;
}

static void passgen_free_accounting(void *state, void *ptr) {
    struct passgen_mem_accounting *acc = state;
    struct passgen_mem_accounting_list *node;

    // go through the allocated blocks to find the one we are trying to free
    for(node = acc->list; node; node = node->next) {
        if(ptr == node->pointer) {
            if(node->freed) {
                // trouble! pointer freed twice.
                passgen_mem_accounting_error(
                    PASSGEN_MEM_ACCOUNTING_DOUBLEFREE,
                    node);
            }

            // mark this as freed, free memory
            node->freed = true;
            passgen_free(acc->mem, ptr);

            // keep track of where this was freed
#if defined(PASSGEN_DEBUG) && defined(PASSGEN_BACKTRACE)
            node->free_frames = backtrace(node->free_bt, sizeof(node->free_bt));
#endif

            return;
        }
    }

    // trouble! pointer unknown.
    passgen_mem_accounting_error(PASSGEN_MEM_ACCOUNTING_FREEBADPTR, ptr);
}

passgen_mem_t passgen_mem_limited(passgen_mem_limits_t *limits) {
    return (passgen_mem_t){
        .malloc = passgen_malloc_limited,
        .realloc = passgen_realloc_limited,
        .calloc = passgen_calloc_limited,
        .free = passgen_free_limited,
        .state = limits,
    };
}

passgen_mem_limits_t
passgen_mem_limits(passgen_mem_t *mem, size_t size, size_t allocs) {
    return (struct passgen_mem_limits){
        .mem = mem,
        .size = 0,
        .size_limit = size,
        .allocs = 0,
        .allocs_limit = allocs,
    };
}

passgen_mem_accounting_t passgen_mem_accounting_new(passgen_mem_t *mem) {
    return (passgen_mem_accounting_t){
        .mem = mem,
        .list = NULL,
    };
}

passgen_mem_t passgen_mem_accounting(passgen_mem_accounting_t *acc) {
    return (passgen_mem_t){
        .malloc = passgen_malloc_accounting,
        .realloc = passgen_realloc_accounting,
        .calloc = passgen_calloc_accounting,
        .free = passgen_free_accounting,
        .state = acc};
}

void passgen_mem_accounting_cleanup(passgen_mem_accounting_t *acc) {
    struct passgen_mem_accounting_list *node, *next;

    // iterate through the nodes and free them
    for(node = acc->list; node; node = next) {
        // free memory if it hasn't already been
        if(!node->freed) {
            fprintf(stderr, "Cleanup: freeing %p\n", node->pointer);

#if defined(PASSGEN_DEBUG) && defined(PASSGEN_BACKTRACE)
            // print allocation backtrace
            fprintf(stderr, "\nBacktrace (allocation):\n");
            char **symbols =
                backtrace_symbols(node->alloc_bt, node->alloc_frames);
            for(size_t i = 0; i < node->alloc_frames; i++) {
                printf("%s\n", symbols[i]);
            }
            free(symbols);
#endif

            passgen_free(acc->mem, node->pointer);
        }

        next = node->next;

        // free node itself
        passgen_free(acc->mem, node);
    }

    // reset accounting list
    acc->list = NULL;
}

bool passgen_mem_accounting_check(passgen_mem_accounting_t *acc) {
    struct passgen_mem_accounting_list *node;

    // iterate through the nodes and check if memory is freed.
    for(node = acc->list; node; node = node->next) {
        if(!node->freed) {
            return false;
        }
    }

    return true;
}
