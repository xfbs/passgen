#include <passgen/assert.h>
#include <passgen/parser/parser.h>
#include <passgen/util/try.h>
#include <passgen/util/utf8.h>
#include <passgen/wordlist.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define REPETITIONS 100
#define GENERATIONS 10

int main(int argc, char *argv[]) {
    (void) argc;

    passgen_assert(argc == 2);

    const char *patterns_path = argv[1];
    passgen_assert(patterns_path);

    FILE *patterns_file = fopen(patterns_path, "r");
    passgen_assert(patterns_file);

    passgen_wordlist patterns;
    passgen_wordlist_load(&patterns, patterns_file, 3);
    fclose(patterns_file);

    // double unicode_time;
    // double tokenize_time;
    // double parse_time;
    // double generate_time;
    // double free_time;

    for(size_t i = 0; i < patterns.count; i++) {
        const char *pattern = patterns.words[i];
        passgen_assert(pattern);
        printf("%s\n", pattern);

        for(size_t r = 0; r < REPETITIONS; r++) {
            uint32_t format_decoded[256];
            size_t format_decoded_len = 0;
            size_t format_bytes_read = 0;
            size_t format_input_len = strlen(pattern);

            try(passgen_utf8_decode(
                format_decoded,
                256,
                &format_decoded_len,
                NULL,
                (unsigned char *) pattern,
                format_input_len,
                &format_bytes_read));
        }
    }

    passgen_wordlist_free(&patterns);

    return EXIT_SUCCESS;
}
