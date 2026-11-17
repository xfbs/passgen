/// @file standalone.c
///
/// Runs a fuzzing harness over the files and directories given as arguments,
/// once each. This is used to run the harnesses over their corpus as part of
/// the tests, without needing a fuzzing engine.
#include <dirent.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);

static size_t count = 0;

static int run_file(const char *path) {
    FILE *file = fopen(path, "rb");
    if(!file) {
        fprintf(stderr, "cannot open %s\n", path);
        return 1;
    }

    size_t capacity = 4096;
    size_t size = 0;
    uint8_t *data = malloc(capacity);
    while(data) {
        size += fread(data + size, 1, capacity - size, file);
        if(size < capacity) {
            break;
        }
        capacity *= 2;
        uint8_t *larger = realloc(data, capacity);
        if(!larger) {
            free(data);
        }
        data = larger;
    }
    fclose(file);

    if(!data) {
        fprintf(stderr, "out of memory reading %s\n", path);
        return 1;
    }

    LLVMFuzzerTestOneInput(data, size);
    free(data);
    count++;

    return 0;
}

static int run_path(const char *path) {
    struct stat info;
    if(0 != stat(path, &info)) {
        fprintf(stderr, "cannot find %s\n", path);
        return 1;
    }

    if(!S_ISDIR(info.st_mode)) {
        return run_file(path);
    }

    DIR *dir = opendir(path);
    if(!dir) {
        fprintf(stderr, "cannot open %s\n", path);
        return 1;
    }

    int ret = 0;
    struct dirent *entry;
    while(ret == 0 && (entry = readdir(dir))) {
        if(entry->d_name[0] == '.') {
            continue;
        }

        size_t length = strlen(path) + strlen(entry->d_name) + 2;
        char *child = malloc(length);
        if(!child) {
            ret = 1;
            break;
        }
        snprintf(child, length, "%s/%s", path, entry->d_name);
        ret = run_path(child);
        free(child);
    }
    closedir(dir);

    return ret;
}

int main(int argc, char *argv[]) {
    for(int i = 1; i < argc; i++) {
        if(0 != run_path(argv[i])) {
            return EXIT_FAILURE;
        }
    }

    printf("ran %zu inputs\n", count);

    // an empty corpus is most likely a mistake.
    return count ? EXIT_SUCCESS : EXIT_FAILURE;
}
