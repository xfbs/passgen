/* Convert token parser data structures into JSON. Used for inspecing and
 * serializing the state.
 */

#pragma once

#include "token_new.h"
#include <jansson.h>

json_t *token_parser_to_json(struct token_parser *parser);

json_t *token_state_to_json(enum token_state state);

json_t *token_type_to_json(enum token_type type);

json_t *token_escaped_to_json(enum token_escaped escaped);

json_t *token_to_json(struct token *token);
