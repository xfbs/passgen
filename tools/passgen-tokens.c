#include <jansson.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "passgen/data/token.h"
#include "passgen/json/token.h"
#include "passgen/token.h"

int main(int argc, char *argv[]) {
  unsigned char stdin_buffer[128];
  uint32_t codepoint_buffer[128];
  size_t stdin_buffer_len;

  (void) argc;
  (void) argv;
  (void) codepoint_buffer;

  while((stdin_buffer_len =
             fread(stdin_buffer, 1, sizeof(stdin_buffer), stdin)) > 0) {
    if(ferror(stdin)) {
      break;
    }

    printf("%zu\n", stdin_buffer_len);
  }

  return EXIT_SUCCESS;
}

void usage(const char *prog) {
  fprintf(stderr, "Usage: %s <string>\n", prog);
  fprintf(stderr, "Decodes the tokens and prints them out as JSON.\n");
}

void dump(const unsigned char *str) {
  struct passgen_token_parser parser = {0};
  struct passgen_token token = {0};

  json_t *output = json_array();

  for(size_t i = 0; str[i]; i++) {
    json_t *obj = json_object();

    json_object_set_new(obj, "character", json_integer(str[i]));

    int ret = passgen_token_parse(&parser, &token, str[i]);
    (void) ret;

    json_t *parser_json = passgen_token_parser_to_json(&parser);
    json_object_set_new(obj, "parser", parser_json);

    if(parser.state == PASSGEN_TOKEN_INIT) {
      json_t *token_json = passgen_token_to_json(&token);
      json_object_set_new(obj, "token", token_json);
    }

    json_array_append_new(output, obj);
  }

  json_dumpf(output, stdout, JSON_INDENT(4));
  json_decref(output);
}
