#include <stdio.h>
#include <jansson.h>
#include "passgen/token_new.h"
#include "passgen/token_new_json.h"

void usage();
void dump(const unsigned char *str);

int main(int argc, char *argv[]) {
    if(argc < 2) {
        usage(argv[0]);
        exit(EXIT_FAILURE);
    } else {
        dump((unsigned char *) argv[1]);
    }

    return EXIT_SUCCESS;
}

void usage(const char *prog) {
    fprintf(stderr, "Usage: %s <string>\n", prog);
    fprintf(stderr, "Decodes the tokens and prints them out as JSON.\n");
}

void dump(const unsigned char *str) {
    struct token_parser parser = {0};
    struct token token = {0};

    json_t *output = json_array();

    for(size_t i = 0; str[i]; i++) {
        json_t *obj = json_object();

        json_object_set_new(obj, "character", json_integer(str[i]));

        int ret = token_parse(&parser, &token, str[i]);

        json_t *parser_json = token_parser_to_json(&parser);
        json_object_set_new(obj, "parser", parser_json);

        if(parser.state == TOKEN_INIT) {
            json_t *token_json = token_to_json(&token);
            json_object_set_new(obj, "token", token_json);
        }

        json_array_append_new(output, obj);
    }

    json_dumpf(output, stdout, JSON_INDENT(4));
    json_decref(output);
}
