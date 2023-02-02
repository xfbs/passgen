#include "passgen/pattern/env.h"
#include "passgen/util/utf8.h"
#include "passgen/wordlist.h"
#include <stdlib.h>
#include <string.h>

void passgen_env_init(passgen_env *env, passgen_random *random) {
    env->entropy = 0;
    env->find_entropy = false;
    env->random = random;
    passgen_hashmap_init(&env->wordlists, &passgen_hashmap_context_utf8);
    passgen_hashmap_init(&env->presets, &passgen_hashmap_context_utf8);
}

static int wordlist_entry_free(void *user, passgen_hashmap_entry *entry) {
    (void) user;
    free((void *) entry->key);
    passgen_wordlist_free(entry->value);
    free(entry->value);
    return 0;
}

void passgen_env_free(passgen_env *env) {
    if(env->random) {
        passgen_random_free(env->random);
    }

    passgen_hashmap_foreach(&env->wordlists, NULL, wordlist_entry_free);
    passgen_hashmap_free(&env->wordlists);
    passgen_hashmap_free(&env->presets);
}

int passgen_env_wordlist_add(
    passgen_env *env,
    char *name,
    FILE *file,
    size_t markov_length) {
    passgen_wordlist *wordlist = malloc(sizeof(passgen_wordlist));
    passgen_hashmap_insert(&env->wordlists, name, wordlist);
    passgen_wordlist_init(wordlist, file, markov_length);

    return 0;
}

int passgen_env_config(passgen_env env, FILE *file) {
    (void) env;
    (void) file;
    // TODO: implement
    return 1;
}
