#include "passgen/parser/parser.h"
#include "passgen/config.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "passgen/container/stack.h"
#include "passgen/pattern/group.h"
#include "passgen/pattern/literal.h"
#include "passgen/pattern/pattern.h"
#include "passgen/pattern/range.h"
#include "passgen/pattern/repeat.h"
#include "passgen/pattern/segment.h"
#include "passgen/pattern/segment_item.h"
#include "passgen/pattern/set.h"
#include "../try.h"
#include "passgen/util/clear.h"

int passgen_parse_group(
    struct passgen_parser *parser,
    struct passgen_token *token,
    passgen_parser_state *state);

int passgen_parse_multiplier(
    struct passgen_parser *parser,
    struct passgen_token *token,
    passgen_parser_state *state);

int passgen_parse_set(
    struct passgen_parser *parser,
    struct passgen_token *token,
    passgen_parser_state *state);

int passgen_parse_set_range(
    struct passgen_parser *parser,
    struct passgen_token *token,
    passgen_parser_state *state);

int passgen_parse_repeat(
    struct passgen_parser *parser,
    struct passgen_token *token,
    passgen_parser_state *state);

int passgen_parse_repeat_range(
    struct passgen_parser *parser,
    struct passgen_token *token,
    passgen_parser_state *state);

int passgen_parse_special(
    struct passgen_parser *parser,
    struct passgen_token *token,
    passgen_parser_state *state);

int passgen_parse_special_name(
    struct passgen_parser *parser,
    struct passgen_token *token,
    passgen_parser_state *state);

inline passgen_parser_state *passgen_parser_state_push(passgen_parser *parser) {
    return passgen_stack_push(&parser->state, NULL);
}

passgen_parser_state *passgen_parser_state_push_group(
    passgen_parser *parser,
    passgen_pattern_group *group,
    passgen_pattern_segment *segment) {
    passgen_parser_state *state = passgen_parser_state_push(parser);
    state->type = PASSGEN_PARSER_GROUP;
    state->data.group.group = group;
    state->data.group.segment = segment;

    return state;
}

passgen_parser_state *passgen_parser_state_push_set(
    passgen_parser *parser,
    passgen_pattern_set *set,
    passgen_pattern_range *range) {
    passgen_parser_state *state = passgen_parser_state_push(parser);
    state->type = PASSGEN_PARSER_SET;
    state->data.set.set = set;
    state->data.set.range = range;

    return state;
}

passgen_parser_state *passgen_parser_state_push_repeat(
    passgen_parser *parser,
    passgen_pattern_repeat *repeat) {
    passgen_parser_state *state = passgen_parser_state_push(parser);
    state->type = PASSGEN_PARSER_REPEAT;
    repeat->min = 0;
    repeat->max = 0;
    state->data.repeat.repeat = repeat;

    return state;
}

passgen_parser_state *passgen_parser_state_push_multiplier(
    passgen_parser *parser,
    size_t *multiplier,
    size_t *sum) {
    passgen_parser_state *state = passgen_parser_state_push(parser);
    state->type = PASSGEN_PARSER_MULTIPLIER;
    *multiplier = 0;
    state->data.multiplier.value = multiplier;
    state->data.multiplier.sum = sum;
    return state;
}

passgen_parser_state *passgen_parser_state_push_special(
    passgen_parser *parser,
    passgen_pattern_special *special) {
    passgen_parser_state *state = passgen_parser_state_push(parser);
    state->type = PASSGEN_PARSER_SPECIAL;
    state->data.special.special = special;
    return state;
}

void passgen_parser_init(passgen_parser *parser, passgen_pattern *pattern) {
    passgen_stack_init(&parser->state, sizeof(passgen_parser_state));
    parser->limit = 0;
    parser->pattern = pattern;
    if(!pattern) {
        parser->pattern = malloc(sizeof(passgen_pattern));
    }
    passgen_pattern_init(parser->pattern);
    passgen_parser_state_push_group(
        parser,
        &parser->pattern->group,
        passgen_pattern_group_segment_append(&parser->pattern->group));
}

passgen_pattern *passgen_parser_free(passgen_parser *parser) {
    passgen_stack_free(&parser->state);
    passgen_pattern *pattern = parser->pattern;
    parser->pattern = NULL;

    PASSGEN_CLEAR(parser);
    return pattern;
}

passgen_parser_state *
passgen_parser_state_get(passgen_parser *parser, size_t n) {
    return passgen_stack_get(&parser->state, n);
}

passgen_parser_state *passgen_parser_state_last(passgen_parser *parser) {
    return passgen_stack_top(&parser->state);
}

// get the last item, making sure that it's only a single character.
// in case of characters, mark it as tainted.
static inline passgen_pattern_item *
last_single_item_taint(passgen_pattern_segment *segment) {
    passgen_pattern_item *item = passgen_stack_top(&segment->items);

    if(!item) {
        return NULL;
    }

    if(item->kind == PASSGEN_PATTERN_LITERAL) {
        if(item->data.literal.count > 1) {
            // save last codepoint
            int32_t codepoint =
                item->data.literal.codepoints[item->data.literal.count - 1];

            // trim codepoints
            item->data.literal.count -= 1;

            // create new item
            item = passgen_pattern_segment_new_item(segment);
            item->kind = PASSGEN_PATTERN_LITERAL;
            passgen_pattern_literal_init(&item->data.literal);
            passgen_pattern_literal_append(&item->data.literal, codepoint);
        }

        // characters are always marked as tainted.
        passgen_pattern_literal_taint(&item->data.literal);
    }

    return item;
}

int passgen_parse_token(passgen_parser *parser, passgen_token *token) {
    passgen_parser_state *state = passgen_parser_state_last(parser);

    if(parser->limit && parser->state.len >= parser->limit) {
        return PASSGEN_ERROR_DEPTH_LIMIT;
    }

    switch(state->type) {
        case PASSGEN_PARSER_GROUP:
            return passgen_parse_group(parser, token, state);
        case PASSGEN_PARSER_MULTIPLIER:
            return passgen_parse_multiplier(parser, token, state);
        case PASSGEN_PARSER_SET:
            return passgen_parse_set(parser, token, state);
        case PASSGEN_PARSER_SET_RANGE:
            return passgen_parse_set_range(parser, token, state);
        case PASSGEN_PARSER_REPEAT:
            return passgen_parse_repeat(parser, token, state);
        case PASSGEN_PARSER_REPEAT_RANGE:
            return passgen_parse_repeat_range(parser, token, state);
        case PASSGEN_PARSER_SPECIAL:
            return passgen_parse_special(parser, token, state);
        case PASSGEN_PARSER_SPECIAL_NAME:
            return passgen_parse_special_name(parser, token, state);
        default:
            return PASSGEN_ERROR_INTERNAL;
    }
}

static void passgen_pattern_segment_clean(passgen_pattern_segment *segment) {
    if(segment->items.len > 0) {
        // get last item
        passgen_pattern_item *last = passgen_stack_top(&segment->items);

        if(last->kind == PASSGEN_PATTERN_GROUP &&
           last->data.group.multiplier_sum == 0) {
            passgen_pattern_group_free(&last->data.group);
            passgen_stack_pop(&segment->items, NULL);
        }
    }
}

int passgen_parse_group(
    passgen_parser *parser,
    passgen_token *token,
    passgen_parser_state *state) {
    uint32_t codepoint = token->codepoint;
    passgen_pattern_group *group;
    passgen_pattern_special *special;
    passgen_pattern_item *item;

    passgen_pattern_segment_clean(state->data.group.segment);

    if(codepoint & PASSGEN_TOKEN_ESCAPED_BIT) {
        codepoint &= ~PASSGEN_TOKEN_ESCAPED_BIT;
        // note that this must compare the whole codepoint: truncating it to
        // a char would mistake any character whose codepoint ends in the
        // same byte (such as U+017C for the pipe, which is U+007C).
        switch(codepoint) {
            case '|':
            case '(':
            case ')':
            case '{':
            case '}':
            case '[':
            case ']':
            case '?':
                // escaped token which would normally do something but
                // should be treated as text
                break;
            case 'm':
            case 'p':
            case 'w':
                // special token
                special = passgen_pattern_segment_new_special(
                    state->data.group.segment);
                passgen_pattern_special_init(special, codepoint);
                passgen_parser_state_push_special(parser, special);
                return 0;
            default:
                // error
                return PASSGEN_ERROR_ESCAPE;
        }
    } else {
        switch(codepoint) {
            case '|':
                if(state->data.group.segment->multiplier > 0) {
                    // create new segment and parser state
                    state->data.group.segment =
                        passgen_pattern_group_segment_append(
                            state->data.group.group);
                } else {
                    // if the previous segment had a zero multiplier, recycle it
                    passgen_pattern_segment_free(state->data.group.segment);
                    passgen_pattern_segment_init(state->data.group.segment);
                }
                return 0;
            case ')':
                if(state->data.group.segment->multiplier == 0) {
                    passgen_pattern_segment_free(state->data.group.segment);
                    passgen_stack_pop(&state->data.group.group->segments, NULL);
                }
                if(parser->state.len <= 1) {
                    return PASSGEN_ERROR_GROUP_UNOPENED;
                }
                passgen_pattern_group_finish(state->data.group.group);
                passgen_stack_pop(&parser->state, NULL);
                return 0;
            case '(':
                // we're supposed to read something in.
                group = passgen_pattern_segment_new_group(
                    state->data.group.segment);
                passgen_parser_state_push_group(
                    parser,
                    group,
                    passgen_pattern_group_segment_append(group));
                return 0;
            case '[':
                passgen_parser_state_push_set(
                    parser,
                    passgen_pattern_segment_new_set(state->data.group.segment),
                    NULL);
                return 0;
            case '{':
                item = last_single_item_taint(state->data.group.segment);
                // error, there was no item
                if(item) {
                    passgen_parser_state_push_repeat(parser, &item->repeat);
                    return 0;
                } else {
                    // the multiplier of this segment is about to be replaced,
                    // so take whatever it is right now out of the sum (it
                    // is not always one: there may have been a multiplier
                    // already, as in `{2}{3}a`).
                    state->data.group.group->multiplier_sum -=
                        state->data.group.segment->multiplier;
                    state->data.group.segment->multiplier = 0;
                    passgen_parser_state_push_multiplier(
                        parser,
                        &state->data.group.segment->multiplier,
                        &state->data.group.group->multiplier_sum);
                    return 0;
                }
            case '?':
                item = last_single_item_taint(state->data.group.segment);
                if(item) {
                    item->maybe = true;
                    return 0;
                } else {
                    // error: maybe without a previous item
                    return PASSGEN_ERROR_OPTIONAL;
                }
            default:
                break;
        }
    }

    // check if the last item was a character that we can add this one to
    if(state->data.group.segment->items.len) {
        passgen_pattern_item *last =
            passgen_stack_top(&state->data.group.segment->items);

        if(last->kind == PASSGEN_PATTERN_LITERAL) {
            if(0 ==
               passgen_pattern_literal_append(&last->data.literal, codepoint)) {
                return 0;
            }
        }
    }

    passgen_pattern_literal *literal =
        passgen_pattern_segment_new_char(state->data.group.segment);
    passgen_pattern_literal_append(literal, codepoint);

    return 0;
}

/// Gets the character that a token stands for inside of a set. The characters
/// which have a meaning in a set can be escaped to get the character itself,
/// and the opening bracket can be for symmetry. Returns nonzero for any other
/// escaped character.
static int passgen_parse_set_codepoint(passgen_token *token, uint32_t *codepoint) {
    *codepoint = token->codepoint;

    if(*codepoint & PASSGEN_TOKEN_ESCAPED_BIT) {
        *codepoint &= ~PASSGEN_TOKEN_ESCAPED_BIT;
        switch(*codepoint) {
            case '-':
            case ']':
            case '[':
                break;
            default:
                return PASSGEN_ERROR_ESCAPE;
        }
    }

    return 0;
}

int passgen_parse_set(
    passgen_parser *parser,
    passgen_token *token,
    passgen_parser_state *state) {
    passgen_pattern_set *set = state->data.set.set;

    // this set's over
    if(token->codepoint == ']') {
        // a set stands for one of its characters, so it needs at least one.
        if(set->items.len == 0) {
            return PASSGEN_ERROR_SET_EMPTY;
        }

        // compute sum of choices and choices list for binary search.
        size_t choices = 0;
        set->choices_list = malloc(sizeof(size_t) * set->items.len);
        for(size_t i = 0; i < set->items.len; i++) {
            passgen_pattern_range *range = passgen_stack_get(&set->items, i);
            choices += 1 + range->end - range->start;
            set->choices_list[i] = choices;
        }

        passgen_stack_pop(&parser->state, NULL);
        return 0;
    }

    // part of a range expression
    if(state->data.set.range && token->codepoint == '-') {
        state->type = PASSGEN_PARSER_SET_RANGE;
        return 0;
    }

    uint32_t codepoint;
    try(passgen_parse_set_codepoint(token, &codepoint));

    passgen_pattern_range *range = passgen_pattern_set_range_append(set);

    range->start = codepoint;
    range->end = codepoint;

    state->data.set.range = range;

    return 0;
}

int passgen_parse_set_range(
    passgen_parser *parser,
    passgen_token *token,
    passgen_parser_state *state) {
    (void) parser;
    if(token->codepoint == ']') {
        return PASSGEN_ERROR_SET_RANGE;
    }

    // the end of the range can be escaped as well. this used to keep the
    // marker for escaped characters as part of the codepoint, resulting in a
    // range which is far too large.
    uint32_t codepoint;
    try(passgen_parse_set_codepoint(token, &codepoint));

    if(codepoint < state->data.set.range->start) {
        return PASSGEN_ERROR_SET_RANGE;
    }

    state->data.set.range->end = codepoint;
    state->type = PASSGEN_PARSER_SET;

    return 0;
}

int passgen_parse_multiplier(
    passgen_parser *parser,
    passgen_token *token,
    passgen_parser_state *state) {
    if(token->codepoint == '}') {
        // the sum of the multipliers must not overflow.
        if(*state->data.multiplier.value >
           SIZE_MAX - *state->data.multiplier.sum) {
            return PASSGEN_ERROR_REPEAT;
        }
        *state->data.multiplier.sum += *state->data.multiplier.value;
        passgen_stack_pop(&parser->state, NULL);
        return 0;
    }

    if(token->codepoint >= '0' && token->codepoint <= '9') {
        uint8_t digit = token->codepoint - '0';

        if(*state->data.multiplier.value > (SIZE_MAX - digit) / 10) {
            return PASSGEN_ERROR_REPEAT;
        }
        *state->data.multiplier.value *= 10;
        *state->data.multiplier.value += digit;

        return 0;
    }

    return PASSGEN_ERROR_REPEAT;
}

int passgen_parse_repeat(
    passgen_parser *parser,
    passgen_token *token,
    passgen_parser_state *state) {
    // this set's over
    if(token->codepoint == '}') {
        state->data.repeat.repeat->max = state->data.repeat.repeat->min;
        passgen_stack_pop(&parser->state, NULL);
        return 0;
    }

    if(token->codepoint == ',') {
        state->data.repeat.repeat->max = 0;
        state->type = PASSGEN_PARSER_REPEAT_RANGE;
        return 0;
    }

    if(token->codepoint >= '0' && token->codepoint <= '9') {
        uint8_t digit = token->codepoint - '0';

        if(state->data.repeat.repeat->min > (SIZE_MAX - digit) / 10) {
            return PASSGEN_ERROR_REPEAT;
        }
        state->data.repeat.repeat->min *= 10;
        state->data.repeat.repeat->min += digit;

        return 0;
    }

    return PASSGEN_ERROR_REPEAT;
}

int passgen_parse_repeat_range(
    passgen_parser *parser,
    passgen_token *token,
    passgen_parser_state *state) {
    if(token->codepoint == '}') {
        // a range like {5,2} makes no sense.
        if(state->data.repeat.repeat->max < state->data.repeat.repeat->min) {
            return PASSGEN_ERROR_REPEAT;
        }
        passgen_stack_pop(&parser->state, NULL);
        return 0;
    }

    if(token->codepoint >= '0' && token->codepoint <= '9') {
        uint8_t digit = token->codepoint - '0';

        if(state->data.repeat.repeat->max > (SIZE_MAX - digit) / 10) {
            return PASSGEN_ERROR_REPEAT;
        }
        state->data.repeat.repeat->max *= 10;
        state->data.repeat.repeat->max += digit;

        return 0;
    }

    return PASSGEN_ERROR_REPEAT;
}

int passgen_parse_special(
    passgen_parser *parser,
    passgen_token *token,
    passgen_parser_state *state) {
    (void) parser;

    if(token->codepoint == '{') {
        state->type = PASSGEN_PARSER_SPECIAL_NAME;
        return 0;
    }

    return PASSGEN_ERROR_SPECIAL;
}

int passgen_parse_special_name(
    passgen_parser *parser,
    passgen_token *token,
    passgen_parser_state *state) {
    (void) parser;

    if(token->codepoint == '}') {
        passgen_stack_pop(&parser->state, NULL);
    } else {
        passgen_pattern_special_push(
            state->data.special.special,
            token->codepoint & ~PASSGEN_TOKEN_ESCAPED_BIT);
    }

    return 0;
}

int passgen_parse_finish(passgen_parser *parser) {
    // make sure we just have one state on the stack, the initial one, which
    // is a group. otherwise, whatever is on top of the stack was not closed.
    passgen_parser_state *state = passgen_parser_state_last(parser);
    if(parser->state.len != 1 || state->type != PASSGEN_PARSER_GROUP) {
        switch(state->type) {
            case PASSGEN_PARSER_GROUP:
                return PASSGEN_ERROR_GROUP_UNCLOSED;
            case PASSGEN_PARSER_SET:
            case PASSGEN_PARSER_SET_RANGE:
                return PASSGEN_ERROR_SET_UNCLOSED;
            case PASSGEN_PARSER_REPEAT:
            case PASSGEN_PARSER_REPEAT_RANGE:
            case PASSGEN_PARSER_MULTIPLIER:
                return PASSGEN_ERROR_REPEAT;
            case PASSGEN_PARSER_SPECIAL:
            case PASSGEN_PARSER_SPECIAL_NAME:
                return PASSGEN_ERROR_SPECIAL;
            default:
                return PASSGEN_ERROR_INTERNAL;
        }
    }

    // clean last state
    passgen_pattern_segment_clean(state->data.group.segment);

    return 0;
}

int passgen_parser_unicode(
    passgen_parser *parser,
    uint32_t *data,
    size_t length) {
    passgen_token_parser token_parser = {0};
    passgen_token token = {0};
    int ret;

    for(size_t pos = 0; pos < length; pos++) {
        ret = passgen_token_parse(&token_parser, &token, 1, data[pos]);

        if(ret == PASSGEN_TOKEN_INIT) {
            ret = passgen_parse_token(parser, &token);

            if(ret != 0) {
                return ret;
            }
        }
    }

    return 0;
}
