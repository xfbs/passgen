/* Convert token parser data structures into JSON. Used for inspecing and
 * serializing the state.
 */

#pragma once

#include <jansson.h>

#include "passgen/token.h"
#include "passgen/data/token.h"

json_t *passgen_token_parser_to_json(struct passgen_token_parser *parser);

json_t *passgen_token_state_to_json(enum passgen_token_state state);

json_t *passgen_token_to_json(struct passgen_token *token);
