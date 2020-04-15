#include "passgen/json/token.h"

json_t *passgen_token_parser_to_json(struct passgen_token_parser *parser) {
  json_t *obj = json_object();

  json_object_set_new(obj, "state", passgen_token_state_to_json(parser->state));

  if(parser->state == PASSGEN_TOKEN_UNICODE_PAYLOAD) {
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

json_t *passgen_token_to_json(struct passgen_token *token) {
  json_t *obj = json_object();

  json_object_set_new(obj, "codepoint", json_integer(token->codepoint));
  json_object_set_new(obj, "escaped", json_boolean(token->escaped));
  json_object_set_new(obj, "type", passgen_token_type_to_json(token->type));

  return obj;
}

json_t *passgen_token_state_to_json(enum passgen_token_state state) {
  for(size_t i = 0; passgen_token_state_enum_by_value[i].name; i++) {
    if(state == passgen_token_state_enum_by_value[i].value) {
      return json_string(passgen_token_state_enum_by_value[i].name);
    }
  }

  return NULL;
}

json_t *passgen_token_type_to_json(enum passgen_token_type type) {
  for(size_t i = 0; passgen_token_type_enum_by_value[i].name; i++) {
    if(type == passgen_token_type_enum_by_value[i].value) {
      return json_string(passgen_token_type_enum_by_value[i].name);
    }
  }

  return NULL;
}

json_t *passgen_token_escaped_to_json(enum passgen_token_escaped escaped) {
  for(size_t i = 0; passgen_token_escaped_enum_by_value[i].name; i++) {
    if(escaped == passgen_token_escaped_enum_by_value[i].value) {
      return json_string(passgen_token_escaped_enum_by_value[i].name);
    }
  }

  return NULL;
}
