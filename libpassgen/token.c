#include "passgen/token.h"

#include "passgen/data/token.h"
#include "passgen/util.h"

static inline void token_parse_init(
    struct passgen_token_parser *parser,
    struct passgen_token *token,
    uint32_t codepoint) {
  if(codepoint == '\\') {
    parser->state = PASSGEN_TOKEN_ESCAPED;
  } else {
    token->codepoint = codepoint;
    parser->state = PASSGEN_TOKEN_INIT;
  }
}

// Simple ASCII escape map. Don't use this for large (unicode) codepoints.
// Provides efficient O(1) lookup.
static const char simple_escaped[] = {
    0,
    ['a'] = '\a',
    ['b'] = '\b',
    ['e'] = '\033',
    ['f'] = '\f',
    ['n'] = '\n',
    ['r'] = '\r',
    ['t'] = '\t',
    ['v'] = '\v',
    ['\\'] = '\\'};

static inline void token_parse_escaped(
    struct passgen_token_parser *parser,
    struct passgen_token *token,
    uint32_t codepoint) {
  // simple_escaped only covers ASCII, whereas codepoint could be much larger.
  if(codepoint < sizeof(simple_escaped) && simple_escaped[codepoint]) {
    token->codepoint = simple_escaped[codepoint];
    parser->state = PASSGEN_TOKEN_INIT;

    return;
  }

  switch(codepoint) {
    case 'u':
      parser->state = PASSGEN_TOKEN_UNICODE;
      break;
    default:
      token->codepoint = codepoint | PASSGEN_TOKEN_ESCAPED_BIT;
      parser->state = PASSGEN_TOKEN_INIT;
  }
}

static inline void
token_parse_unicode(struct passgen_token_parser *parser, uint32_t codepoint) {
  if(codepoint == '{') {
    parser->state = PASSGEN_TOKEN_UNICODE_PAYLOAD;
    parser->data.unicode_payload.length = 0;
    parser->data.unicode_payload.codepoint = 0;
  } else {
    parser->state = PASSGEN_TOKEN_ERROR_UNICODE_START;
  }
}

static inline void token_parse_unicode_payload(
    struct passgen_token_parser *parser,
    struct passgen_token *token,
    uint32_t codepoint) {
  // once we read the closing brace, the payload is over and we can emit the
  // token.
  if(codepoint == '}') {
    token->codepoint = parser->data.unicode_payload.codepoint;
    parser->state = PASSGEN_TOKEN_INIT;

    return;
  }

  // keep track of length, make sure it's not too long.
  parser->data.unicode_payload.length++;
  if(parser->data.unicode_payload.length > 6) {
    parser->state = PASSGEN_TOKEN_ERROR_UNICODE_PAYLOAD_LEN;

    return;
  }

  // try to decode the hex value.
  int8_t decoded = hex_decode(codepoint);
  if(decoded < 0) {
    parser->state = PASSGEN_TOKEN_ERROR_UNICODE_PAYLOAD;

    return;
  }

  parser->data.unicode_payload.codepoint *= 16;
  parser->data.unicode_payload.codepoint += decoded;
}

int passgen_token_parse(
    struct passgen_token_parser *parser,
    struct passgen_token *token,
    uint32_t codepoint) {
  switch(parser->state) {
    case PASSGEN_TOKEN_INIT:
      token_parse_init(parser, token, codepoint);
      break;
    case PASSGEN_TOKEN_ESCAPED:
      token_parse_escaped(parser, token, codepoint);
      break;
    case PASSGEN_TOKEN_UNICODE:
      token_parse_unicode(parser, codepoint);
      break;
    case PASSGEN_TOKEN_UNICODE_PAYLOAD:
      token_parse_unicode_payload(parser, token, codepoint);
      break;
    default:
      break;
  }

  return parser->state;
}

const char *passgen_token_parse_error_str(int ret) {
  // TODO fixme
  (void) ret;

  return "err";
}
