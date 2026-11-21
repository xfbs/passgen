/// @file master.c
///
/// Benchmarks deriving the key from a master passphrase, which is meant to
/// be slow and to use a lot of memory, so this only catches it getting slower
/// by mistake. It only runs when it is asked for by name. The level is 1,
/// unless it is given with `-o level=N`.
#include "bench.h"
#include <passgen/assert.h>
#include <passgen/config.h>
#include <passgen/master.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef PASSGEN_MONOCYPHER
static void *bench_master_prepare(const passgen_hashmap *opts) {
    unsigned *level = malloc(sizeof(unsigned));
    *level = 1;
    passgen_hashmap_entry *entry = passgen_hashmap_lookup(opts, "level");
    if(entry) {
        *level = atoi(entry->value);
    }

    passgen_master_cost cost;
    if(!passgen_master_level(PASSGEN_MASTER_VERSION, *level, &cost)) {
        fprintf(
            stderr,
            "note: skipping master_derive, there is no level %u\n",
            *level);
        free(level);
        return NULL;
    }
    return level;
}

static void *bench_master_iterate(void *raw_data) {
    unsigned *level = raw_data;
    passgen_master master;
    passgen_status status = passgen_master_derive(
        &master,
        PASSGEN_MASTER_VERSION,
        *level,
        "correct horse battery staple",
        NULL,
        0);
    passgen_assert(status == PASSGEN_OK);
    (void) status;
    passgen_master_wipe(&master);
    return NULL;
}

const bench master_derive = {
    .name = "master_derive",
    .desc = "Derives the key from a master passphrase (only when named)",
    .unit = "keys",
    .optional = true,
    .prepare = &bench_master_prepare,
    .iterate = &bench_master_iterate,
    .release = &free,
};
#endif
