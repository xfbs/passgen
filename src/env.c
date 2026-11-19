#include "passgen/pattern/env.h"
#include "passgen/config.h"
#include "passgen/util/alloc.h"
#include "passgen/util/clear.h"
#include "passgen/util/utf8.h"
#include "passgen/wordlist.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

passgen_env *passgen_env_new(passgen_random *random) {
    passgen_env *env = passgen_alloc(sizeof(passgen_env));
    if(!env) {
        return NULL;
    }

    env->step_limit = 1000000;
    env->random = random;
    passgen_hashmap_init(&env->wordlists, &passgen_hashmap_context_utf8);
    passgen_hashmap_init(&env->presets, &passgen_hashmap_context_utf8);

    return env;
}

static int wordlist_entry_free(void *user, passgen_hashmap_entry *entry) {
    (void) user;
    passgen_free_string((char *) entry->key);
    passgen_wordlist_free(entry->value);
    passgen_free(entry->value, sizeof(passgen_wordlist));
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
    passgen_free(env, sizeof(passgen_env));
}

void passgen_env_random_set(passgen_env *env, passgen_random *random) {
    env->random = random;
}

void passgen_env_step_limit_set(passgen_env *env, size_t step_limit) {
    env->step_limit = step_limit;
}

passgen_status passgen_env_wordlist_add(
    passgen_env *env,
    const char *name,
    FILE *file,
    size_t markov_length) {
    // a Markov chain looks at between one and 255 codepoints (it keeps the
    // length in a byte).
    if(markov_length < 1 || markov_length > UINT8_MAX) {
        // the file is ours to close, whether this works or not.
        fclose(file);
        return PASSGEN_ERROR_INVALID_ARGUMENT;
    }

    char *name_copy = passgen_alloc_string(name);
    passgen_wordlist *wordlist = passgen_alloc(sizeof(passgen_wordlist));
    if(!name_copy || !wordlist) {
        // the file is ours to close, whether this works or not.
        passgen_free_string(name_copy);
        passgen_free(wordlist, sizeof(passgen_wordlist));
        fclose(file);
        return PASSGEN_ERROR_OUT_OF_MEMORY;
    }

    passgen_wordlist_init(wordlist, file, markov_length);

    // if a wordlist with this name was already defined, the new one replaces
    // it.
    passgen_hashmap_entry old;
    if(PASSGEN_OK !=
       passgen_hashmap_insert(&env->wordlists, name_copy, wordlist, &old)) {
        // this closes the file.
        passgen_wordlist_free(wordlist);
        passgen_free(wordlist, sizeof(passgen_wordlist));
        passgen_free_string(name_copy);
        return PASSGEN_ERROR_OUT_OF_MEMORY;
    }
    if(old.key) {
        wordlist_entry_free(NULL, &old);
    }

    return PASSGEN_OK;
}
