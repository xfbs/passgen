#include "passgen/pattern/env.h"
#include "passgen/pattern/preset.h"
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
    env->version = 0;
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

static int preset_entry_free(void *user, passgen_hashmap_entry *entry) {
    (void) user;
    passgen_free_string((char *) entry->key);
    passgen_preset_free(entry->value);
    passgen_free(entry->value, sizeof(passgen_preset));
    return 0;
}

void passgen_env_free(passgen_env *env) {
    if(!env) {
        return;
    }

    passgen_hashmap_foreach(&env->wordlists, NULL, wordlist_entry_free);
    passgen_hashmap_free(&env->wordlists);
    passgen_hashmap_foreach(&env->presets, NULL, preset_entry_free);
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

// Adds a wordlist which was initialized, under a copy of the name. The
// wordlist is freed if this fails.
static passgen_status wordlist_insert(
    passgen_env *env,
    const char *name,
    passgen_wordlist *wordlist) {
    char *name_copy = passgen_alloc_string(name);
    if(!name_copy) {
        passgen_wordlist_free(wordlist);
        passgen_free(wordlist, sizeof(passgen_wordlist));
        return PASSGEN_ERROR_OUT_OF_MEMORY;
    }

    // patterns which were resolved before may point to a wordlist which this
    // replaces, so they have to be resolved again.
    env->version += 1;

    // if a wordlist with this name was already defined, the new one replaces
    // it.
    passgen_hashmap_entry old;
    if(PASSGEN_OK !=
       passgen_hashmap_insert(&env->wordlists, name_copy, wordlist, &old)) {
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

passgen_status
passgen_env_wordlist_add(passgen_env *env, const char *name, FILE *file) {
    passgen_wordlist *wordlist = passgen_alloc(sizeof(passgen_wordlist));
    if(!wordlist) {
        // the file is ours to close, whether this works or not.
        fclose(file);
        return PASSGEN_ERROR_OUT_OF_MEMORY;
    }
    passgen_wordlist_init(wordlist, file);

    // this closes the file if it fails.
    return wordlist_insert(env, name, wordlist);
}

passgen_status passgen_env_wordlist_add_path(
    passgen_env *env,
    const char *name,
    const char *path) {
    char *path_copy = passgen_alloc_string(path);
    passgen_wordlist *wordlist = passgen_alloc(sizeof(passgen_wordlist));
    if(!path_copy || !wordlist) {
        passgen_free_string(path_copy);
        passgen_free(wordlist, sizeof(passgen_wordlist));
        return PASSGEN_ERROR_OUT_OF_MEMORY;
    }
    passgen_wordlist_init_path(wordlist, path_copy);

    return wordlist_insert(env, name, wordlist);
}

const char *passgen_env_wordlist_path(const passgen_env *env, const char *name) {
    passgen_hashmap_entry *entry =
        passgen_hashmap_lookup(&env->wordlists, name);
    if(!entry) {
        return NULL;
    }

    const passgen_wordlist *wordlist = entry->value;
    return wordlist->path;
}

bool passgen_env_wordlist_remove(passgen_env *env, const char *name) {
    passgen_hashmap_entry old = passgen_hashmap_remove(&env->wordlists, name);
    if(!old.key) {
        return false;
    }
    wordlist_entry_free(NULL, &old);

    // patterns which were resolved before may point to it.
    env->version += 1;
    return true;
}

passgen_status
passgen_env_preset_add(passgen_env *env, const char *name, const char *pattern) {
    char *name_copy = passgen_alloc_string(name);
    char *pattern_copy = passgen_alloc_string(pattern);
    passgen_preset *preset = passgen_alloc(sizeof(passgen_preset));
    if(!name_copy || !pattern_copy || !preset) {
        passgen_free_string(name_copy);
        passgen_free_string(pattern_copy);
        passgen_free(preset, sizeof(passgen_preset));
        return PASSGEN_ERROR_OUT_OF_MEMORY;
    }

    passgen_preset_init(preset, pattern_copy);

    // if a preset with this name was already defined, the new one replaces
    // it.
    passgen_hashmap_entry old;
    if(PASSGEN_OK !=
       passgen_hashmap_insert(&env->presets, name_copy, preset, &old)) {
        passgen_preset_free(preset);
        passgen_free(preset, sizeof(passgen_preset));
        passgen_free_string(name_copy);
        return PASSGEN_ERROR_OUT_OF_MEMORY;
    }
    if(old.key) {
        preset_entry_free(NULL, &old);
    }

    // patterns which were resolved before may point to a preset which this
    // replaced, or one which uses it.
    env->version += 1;

    return PASSGEN_OK;
}

bool passgen_env_preset_remove(passgen_env *env, const char *name) {
    passgen_hashmap_entry old = passgen_hashmap_remove(&env->presets, name);
    if(!old.key) {
        return false;
    }
    preset_entry_free(NULL, &old);

    // patterns which were resolved before may point to it, or to a preset
    // which uses it.
    env->version += 1;
    return true;
}

const char *passgen_env_preset_get(const passgen_env *env, const char *name) {
    passgen_hashmap_entry *entry = passgen_hashmap_lookup(&env->presets, name);
    if(!entry) {
        return NULL;
    }

    const passgen_preset *preset = entry->value;
    return preset->value;
}

typedef struct {
    void *user;
    passgen_env_preset_cb *func;
} preset_foreach_context;

static int preset_foreach_entry(void *user, passgen_hashmap_entry *entry) {
    preset_foreach_context *context = user;
    const passgen_preset *preset = entry->value;
    return context->func(context->user, entry->key, preset->value);
}

int passgen_env_preset_foreach(
    const passgen_env *env,
    void *user,
    passgen_env_preset_cb *func) {
    preset_foreach_context context = {user, func};
    return passgen_hashmap_foreach(
        &env->presets,
        &context,
        preset_foreach_entry);
}
