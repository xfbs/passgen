#include "passgen/token_new_json.h"

json_t *token_parser_to_json(struct token_parser *parser) {
  json_t *obj = json_object();

  json_object_set_new(obj, "state", token_state_to_json(parser->state));

  if(parser->state == TOKEN_UNICODE_PAYLOAD) {
    json_t *data = json_object();

    json_object_set_new(
        data,
        "length",
        json_integer(parser->data.unicode_payload.length));
    json_object_set_new(
        data,
        "codepoint",
        json_integer(parser->data.unicode_payload.codepoint));

    json_object_set_new(obj, "unicode_payload", data);
  }

  return obj;
}

json_t *token_to_json(struct token *token) {
  json_t *obj = json_object();

  json_object_set_new(obj, "codepoint", json_integer(token->codepoint));
  json_object_set_new(obj, "escaped", json_boolean(token->escaped));
  json_object_set_new(obj, "type", token_type_to_json(token->type));

  return obj;
}

json_t *token_state_to_json(enum token_state state) {
  for(size_t i = 0; token_state_mapping[i].name; i++) {
    if(state == token_state_mapping[i].value) {
      return json_string(token_state_mapping[i].name);
    }
  }

  return NULL;
}

json_t *token_type_to_json(enum token_type type) {
  for(size_t i = 0; token_type_mapping[i].name; i++) {
    if(type == token_type_mapping[i].value) {
      return json_string(token_type_mapping[i].name);
    }
  }

  return NULL;
}

json_t *token_escaped_to_json(enum token_escaped escaped) {
  for(size_t i = 0; token_escaped_mapping[i].name; i++) {
    if(escaped == token_escaped_mapping[i].value) {
      return json_string(token_escaped_mapping[i].name);
    }
  }

  return NULL;
}
