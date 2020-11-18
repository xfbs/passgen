#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "passgen/utf8.h"

int main(int argc, char *argv[]) {
     uint8_t utf8_input[128];
     size_t utf8_input_start = 0;
     size_t utf8_input_end = 0;
     uint32_t decoded[128];
     size_t decoded_pos = 0;
     uint8_t utf8_output[128];
     int ret;

     (void) argc;
     (void) argv;
     (void) utf8_output;

     while(true) {
          if(ferror(stdin)) {
               goto error;
          }

          if(feof(stdin)) {
               break;
          }

          // read a chunk of data into the utf8_input buffer
          utf8_input_end += fread(
              utf8_input + utf8_input_end,
              sizeof(utf8_input[0]),
              sizeof(utf8_input) - utf8_input_end,
              stdin);

          printf("read %zu bytes\n", utf8_input_end);

          ret = passgen_utf8_decode(
              decoded,
              sizeof(decoded),
              &decoded_pos,
              utf8_input,
              utf8_input_end - utf8_input_start,
              &utf8_input_start);

          printf("ret is %i\n", ret);

          printf("read %zu characters\n", decoded_pos);

          decoded_pos = 0;
          utf8_input_start = 0;
          utf8_input_end = 0;
     }

     return EXIT_SUCCESS;

error:
     return EXIT_FAILURE;
}
