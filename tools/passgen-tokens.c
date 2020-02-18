#include "passgen/token_new.h"
#include "passgen/token_new_json.h"
#include <stdio.h>

void usage();
void dump(const char *str);

int main(int argc, char *argv[]) {
    if(argc < 2) {
        usage(argv[0]);
        exit(EXIT_FAILURE);
    } else {
        dump(argv[1]);
    }

    return EXIT_SUCCESS;
}

void usage(const char *prog) {
    fprintf(stderr, "Usage: %s <string>\n", prog);
    fprintf(stderr, "Decodes the tokens and prints them out as JSON.\n");
}

void dump(const char *str) {
    struct token_parser parser = {0};
    struct token token = {0};

    for(size_t i = 0; str[i]; i++) {
        printf("Parsing character %i\n", str[i]);

        int ret = token_parse(&parser, &token, (unsigned char) str[i]);
        json_t *obj = token_parser_to_json(&parser);

        json_dumpf(obj, stdout, JSON_INDENT(4));
        printf("\n");

        json_decref(obj);

        if(parser.state == TOKEN_INIT) {
            printf("Got token\n");

            obj = token_to_json(&token);

            json_dumpf(obj, stdout, JSON_INDENT(4));
            printf("\n");

            json_decref(obj);
        }
    }
}
