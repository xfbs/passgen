#include "passgen/data/token.h"

void passgen_token_parser_init(struct passgen_token_parser *token_parser) {
  token_parser->state = PASSGEN_TOKEN_INIT;
}
