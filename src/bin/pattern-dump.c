#include "passgen/pattern.h"
#include <stdio.h>

void pattern_dump(pattern_t *pattern, int indent);
void pattern_segment_dump(pattern_segment_t *segment, int indent);
void pattern_segment_char_dump(pattern_segment_t *segment, int indent);
void pattern_segment_group_dump(pattern_segment_t *segment, int indent);
void pattern_segment_range_dump(pattern_segment_t *segment, int indent);
void pattern_reps_dump(pattern_reps_t reps, int indent);

int main(int argc, char *argv[]) {
  (void) argc;

  if(argc == 1) {
    fprintf(stderr, "Usage: %s PATTERN\n", argv[0]);
    fprintf(stderr, "Dumps out the given pattern.\n");
    exit(-1);
  }

  const char *pattern_str = argv[1];
  pattern_t *pattern = pattern_parse(&pattern_str);

  if(!pattern) {
    int error_pos = pattern_str - argv[1];
    fprintf(stderr, "Error parsing pattern: %s\n", argv[1]);
    fprintf(stderr, "%*c^", error_pos + 13, ' ');
    exit(-1);
  }

  if(pattern_str[0] != '\0') {
    int error_pos = pattern_str - argv[1];
    fprintf(stderr, "Error parsing pattern: '%s'\n", argv[1]);
    fprintf(stderr, "%*cerror likely here ^\n", error_pos + 6, ' ');
  }

  pattern_dump(pattern, 0);

  if(pattern) {
    pattern_free(pattern);
  }


  return 0;
}

void pattern_dump(pattern_t *pattern, int indent) {
  if(!pattern) {
    printf("%*sgroup NULL\n", indent, "");
    return;
  }

  printf("%*spattern {\n", indent, "");

  while(pattern) {
    pattern_segment_dump(pattern->item, indent + 2);
    pattern = pattern->next;
  }

  printf("%*s}\n", indent, "");
}

void pattern_segment_dump(pattern_segment_t *segment, int indent) {
  if(!segment) {
    printf("%*ssegment NULL\n", indent, "");
    return;
  }

  while(segment) {
    switch(segment->kind) {
      case PATTERN_CHAR:
        pattern_segment_char_dump(segment, indent);
        break;
      case PATTERN_RANGE:
        break;
      case PATTERN_GROUP:
        pattern_segment_group_dump(segment, indent);
        break;
    }

    segment = segment->next;
  }
}

void pattern_segment_char_dump(pattern_segment_t *segment, int indent) {
  printf("%*schar {\n", indent, "");

  if(segment->data.chr) {
    printf("%*sdata '%c'\n", indent + 2, "", segment->data.chr[0]);
  } else {
    printf("%*sdata NULL\n", indent + 2, "");
  }

  pattern_reps_dump(segment->reps, indent + 2);
  printf("%*s}\n", indent, "");
}

void pattern_segment_group_dump(pattern_segment_t *segment, int indent) {
  pattern_t *pattern = segment->data.group;

  if(!pattern) {
    printf("%*sgroup NULL\n", indent, "");
    return;
  }

  printf("%*sgroup {\n", indent, "");
  while(pattern) {
    pattern_segment_dump(pattern->item, indent + 2);
    pattern = pattern->next;
  }

  pattern_reps_dump(segment->reps, indent + 2);
  printf("%*s}\n", indent, "");
}

void pattern_reps_dump(pattern_reps_t reps, int indent) {
  if(reps.min != 1 || reps.max != 1) {
    if(reps.min == reps.max) {
      printf("%*srepeat %zi\n", indent, "", reps.min);
    } else {
      printf("%*srepeat %zi-%zi\n", indent, "", reps.min, reps.max);
    }
  }
}
