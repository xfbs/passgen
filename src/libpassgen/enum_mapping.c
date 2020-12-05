#include "passgen/enum_mapping.h"
#include <stdlib.h>
#include <string.h>

static int enum_mapping_namecmp(const void *key, const void *item) {
     const char *name = key;
     const struct passgen_enum_mapping *mapping = item;

     return strcmp(name, mapping->name);
}

static int enum_mapping_valuecmp(const void *key, const void *item) {
     const int *value = key;
     const struct passgen_enum_mapping *mapping = item;

     return (*value < mapping->value) ? -1
                                      : ((*value == mapping->value) ? 0 : 1);
}

struct passgen_enum_mapping *passgen_enum_by_name(
    const struct passgen_enum_mapping mapping[],
    size_t count,
    const char *name) {
     return bsearch(
         name,
         mapping,
         count,
         sizeof(struct passgen_enum_mapping),
         enum_mapping_namecmp);
}

struct passgen_enum_mapping *passgen_enum_by_value(
    const struct passgen_enum_mapping mapping[],
    size_t count,
    int value) {
     return bsearch(
         &value,
         mapping,
         count,
         sizeof(struct passgen_enum_mapping),
         enum_mapping_valuecmp);
}
