#include <stdio.h>
#include <string.h>

#include "passgen/data/array.h"
#include "passgen/data/chars.h"
#include "passgen/data/group.h"
#include "passgen/data/parser.h"
#include "passgen/data/parser_state.h"
#include "passgen/data/pattern.h"
#include "passgen/data/pattern_kind.h"
#include "passgen/data/range.h"
#include "passgen/data/repeat.h"
#include "passgen/data/segment.h"
#include "passgen/data/segment_item.h"
#include "passgen/data/set.h"
#include "passgen/data/special.h"
#include "passgen/data/special_kind.h"
#include "passgen/data/token.h"
#include "passgen/markov.h"
#include "passgen/pronounceable.h"

#define add_type(name) \
    { #name, sizeof(name) }

#define add_struct(name) \
    { #name, sizeof(struct name) }

#define size_info_end() \
    { NULL, 0 }

struct size_info {
    const char *name;
    size_t size;
};

static const struct size_info type_sizes[] = {
    add_type(size_t),
    add_type(void *),
    size_info_end()};

static const struct size_info struct_sizes[] = {
    add_struct(passgen_parser),
    add_struct(passgen_parser_state),
    add_struct(passgen_pattern),
    add_struct(passgen_pattern_repeat),
    add_struct(passgen_pattern_group),
    add_struct(passgen_pattern_segment),
    add_struct(passgen_pattern_item),
    add_struct(passgen_chars),
    add_struct(passgen_pattern_set),
    add_struct(passgen_pattern_special),
    add_struct(passgen_token),
    add_struct(passgen_token_parser),
    add_struct(passgen_markov3),
    add_struct(passgen_markov0),
    add_struct(passgen_markov1),
    add_struct(passgen_markov2),
    size_info_end()};

void show_size_info(const struct size_info size_info[]) {
    // figure out length of biggest element
    size_t max_len = 0;
    for(size_t i = 0; size_info[i].name; i++) {
        size_t len = strlen(size_info[i].name);

        if(len > max_len) {
            max_len = len;
        }
    }

    for(size_t i = 0; size_info[i].name; i++) {
        size_t len = strlen(size_info[i].name);
        int padding = 1 + max_len - len;
        printf(
            "sizeof(%s)%*c= %zu\n",
            size_info[i].name,
            padding,
            ' ',
            size_info[i].size);
    }
}

int main(int argc, char *argv[]) {
    (void) argc;
    (void) argv;

    printf("size of built-in types:\n");
    show_size_info(type_sizes);

    printf("\nsize of data structures:\n");
    show_size_info(struct_sizes);

    printf("\nsize of markov chains:\n");
    // TODO

    return 0;
}
