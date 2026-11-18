#include "passgen/pattern/set.h"
#include "passgen/config.h"
#include "passgen/pattern/range.h"
#include "passgen/util/alloc.h"
#include "passgen/util/clear.h"
#include <passgen/error.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

void passgen_pattern_set_init(passgen_pattern_set *set) {
    passgen_stack_init(&set->items, sizeof(passgen_pattern_range));

    set->choices_list = NULL;
}

void passgen_pattern_set_free(passgen_pattern_set *set) {
    // the list of choices has an entry for every range, once the set is
    // complete.
    passgen_free(set->choices_list, set->items.len * sizeof(size_t));
    passgen_stack_free(&set->items);
    PASSGEN_CLEAR(set);
}

passgen_pattern_range *
passgen_pattern_set_range_append(passgen_pattern_set *set) {
    return passgen_stack_push(&set->items, NULL);
}

passgen_pattern_range *
passgen_pattern_set_range_get(passgen_pattern_set *set, size_t n) {
    return passgen_stack_get(&set->items, n);
}

/// One end of a range, for sweeping over all of the ranges of a set: the
/// weight goes up by one where a range starts, and down by one after it ends.
typedef struct {
    uint32_t codepoint;
    int32_t change;
} set_event;

static int set_event_compare(const void *a, const void *b) {
    uint32_t left = ((const set_event *) a)->codepoint;
    uint32_t right = ((const set_event *) b)->codepoint;
    return (left > right) - (left < right);
}

/// Appends a range with a weight to a stack of ranges, merging it with the
/// previous one if they are adjacent and have the same weight.
static bool set_range_push(
    passgen_stack *ranges,
    uint32_t start,
    uint32_t end,
    size_t weight) {
    passgen_pattern_range *last = passgen_stack_top(ranges);
    if(last && last->end + 1 == start && last->multiplier == weight) {
        last->end = end;
        return true;
    }

    passgen_pattern_range *range = passgen_stack_push(ranges, NULL);
    if(!range) {
        return false;
    }
    range->start = start;
    range->end = end;
    range->multiplier = weight;
    return true;
}

int passgen_pattern_set_finish(passgen_pattern_set *set) {
    size_t count = 2 * set->items.len;
    set_event *events = passgen_alloc(count * sizeof(set_event));
    if(!events) {
        return PASSGEN_ERROR_OUT_OF_MEMORY;
    }
    for(size_t i = 0; i < set->items.len; i++) {
        passgen_pattern_range *range = passgen_stack_get(&set->items, i);
        // codepoints are at most 0x10ffff, so this cannot overflow.
        events[2 * i] = (set_event){range->start, 1};
        events[2 * i + 1] = (set_event){range->end + 1, -1};
    }
    qsort(events, count, sizeof(set_event), set_event_compare);

    // sweep over the events in order. between two of them, the weight of
    // every codepoint is how many of the ranges contain it.
    passgen_stack ranges;
    passgen_stack_init(&ranges, sizeof(passgen_pattern_range));
    size_t weight = 0;
    for(size_t i = 0; i < count;) {
        uint32_t codepoint = events[i].codepoint;
        for(; i < count && events[i].codepoint == codepoint; i++) {
            weight += events[i].change;
        }
        if(weight && i < count &&
           !set_range_push(&ranges, codepoint, events[i].codepoint - 1, weight)) {
            passgen_stack_free(&ranges);
            passgen_free(events, count * sizeof(set_event));
            return PASSGEN_ERROR_OUT_OF_MEMORY;
        }
    }
    passgen_free(events, count * sizeof(set_event));

    // the cumulative number of choices up to and including every range, for
    // finding the range of a choice with a binary search.
    size_t *choices_list = passgen_alloc(ranges.len * sizeof(size_t));
    if(!choices_list) {
        passgen_stack_free(&ranges);
        return PASSGEN_ERROR_OUT_OF_MEMORY;
    }
    size_t choices = 0;
    for(size_t i = 0; i < ranges.len; i++) {
        passgen_pattern_range *range = passgen_stack_get(&ranges, i);
        size_t size = (size_t) (range->end - range->start) + 1;
        // only a set with a great many ranges gets here, and only where
        // size_t has 32 bits (such as wasm32).
        if(range->multiplier > (SIZE_MAX - choices) / size) {
            passgen_free(choices_list, ranges.len * sizeof(size_t));
            passgen_stack_free(&ranges);
            return PASSGEN_ERROR_SET_RANGE;
        }
        choices += size * range->multiplier;
        choices_list[i] = choices;
    }

    passgen_stack_free(&set->items);
    set->items = ranges;
    set->choices_list = choices_list;
    return 0;
}
