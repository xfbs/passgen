#include "passgen/pattern/env.h"
#include "passgen/config.h"
#include "passgen/util/clear.h"
#include "passgen/util/utf8.h"
#include "passgen/wordlist.h"
#include <stdlib.h>
#include <string.h>

passgen_env *passgen_env_new(passgen_random *random) {
    passgen_env *env = malloc(sizeof(passgen_env));
    if(!env) {
        return NULL;
    }

    env->depth_limit = 1024;
    env->random = random;
    passgen_hashmap_init(&env->wordlists, &passgen_hashmap_context_utf8);
    passgen_hashmap_init(&env->presets, &passgen_hashmap_context_utf8);

    return env;
}

static int wordlist_entry_free(void *user, passgen_hashmap_entry *entry) {
    (void) user;
    free((void *) entry->key);
    passgen_wordlist_free(entry->value);
    free(entry->value);
    return 0;
}

void passgen_env_free(passgen_env *env) {
    if(!env) {
        return;
    }

    passgen_hashmap_foreach(&env->wordlists, NULL, wordlist_entry_free);
    passgen_hashmap_free(&env->wordlists);
    passgen_hashmap_free(&env->presets);

    PASSGEN_CLEAR(env);
    free(env);
}

void passgen_env_random_set(passgen_env *env, passgen_random *random) {
    env->random = random;
}

void passgen_env_depth_limit_set(passgen_env *env, size_t depth_limit) {
    env->depth_limit = depth_limit;
}

passgen_status passgen_env_wordlist_add(
    passgen_env *env,
    const char *name,
    FILE *file,
    size_t markov_length) {
    size_t name_size = strlen(name) + 1;
    char *name_copy = malloc(name_size);
    passgen_wordlist *wordlist = malloc(sizeof(passgen_wordlist));
    if(!name_copy || !wordlist) {
        // the file is ours to close, whether this works or not.
        free(name_copy);
        free(wordlist);
        fclose(file);
        return PASSGEN_ERROR_OUT_OF_MEMORY;
    }

    memcpy(name_copy, name, name_size);
    passgen_wordlist_init(wordlist, file, markov_length);

    // if a wordlist with this name was already defined, the new one replaces
    // it.
    passgen_hashmap_entry old;
    if(PASSGEN_OK !=
       passgen_hashmap_insert(&env->wordlists, name_copy, wordlist, &old)) {
        // this closes the file.
        passgen_wordlist_free(wordlist);
        free(wordlist);
        free(name_copy);
        return PASSGEN_ERROR_OUT_OF_MEMORY;
    }
    if(old.key) {
        wordlist_entry_free(NULL, &old);
    }

    return PASSGEN_OK;
}
