#include "passgen/pattern/env.h"
#include "passgen/wordlist.h"
#include "passgen/util/utf8.h"
#include <stdlib.h>
#include <string.h>

void passgen_env_init(passgen_env *env, passgen_random *random) {
    env->entropy = 0;
    env->find_entropy = false;
    env->random = random;
    passgen_hashmap_init(&env->wordlists, &passgen_hashmap_context_unicode);
    passgen_hashmap_init(&env->presets, &passgen_hashmap_context_default);
}

static int wordlist_entry_free(void *user, passgen_hashmap_entry *entry) {
    (void) user;
    free(entry->key);
    passgen_wordlist_free(entry->value);
    free(entry->value);
    return 0;
}

void passgen_env_free(passgen_env *env) {
    if(env->random) {
        passgen_random_free(env->random);
    }

    passgen_hashmap_foreach(
        &env->wordlists,
        NULL,
        wordlist_entry_free);
    passgen_hashmap_free(&env->wordlists);
    passgen_hashmap_free(&env->presets);
}

int passgen_env_wordlist_add(passgen_env *env, const char *input, FILE *file, size_t markov_length) {
    // copy name
    size_t name_len = 256;
    uint32_t *name = calloc(name_len, sizeof(int32_t));
    size_t name_len_out = 0;
    size_t input_pos = 0;
    int ret = passgen_utf8_decode(
        name,
        name_len,
        &name_len_out,
        NULL,
        (unsigned char *) input,
        strlen(input),
        &input_pos);
    if(ret != 0) {
        free(name);
        return ret;
    }

    passgen_wordlist *wordlist = malloc(sizeof(passgen_wordlist));
    passgen_hashmap_insert(&env->wordlists, name, wordlist);
    passgen_wordlist_init(wordlist, file, markov_length);

    return 0;
}

int passgen_env_config(passgen_env env, FILE *file) {
    return 1;
}
