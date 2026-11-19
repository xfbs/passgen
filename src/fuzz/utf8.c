/// @file utf8.c
///
/// Fuzzing harness for the UTF-8 decoder and encoder. The input is decoded in
/// chunks. If it is valid, encoding the result again has to give back the
/// input.
#include <passgen/util/utf8.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    // the size of the chunks comes from the input, to cover all of the ways
    // in which a sequence can be split up.
    if(size < 1) {
        return 0;
    }
    size_t chunk = 1 + data[0] % 16;
    data += 1;
    size -= 1;

    uint8_t *encoded = malloc(size + 4);
    if(!encoded) {
        return 0;
    }
    size_t encoded_len = 0;

    const uint8_t *input = data;
    uint32_t codepoints[16];
    uint8_t widths[16];
    int ret;

    do {
        uint32_t *output = &codepoints[0];
        size_t remaining = size - (size_t) (input - data);
        ret = passgen_utf8_decode(&output, chunk, widths, &input, remaining);

        size_t decoded = (size_t) (output - &codepoints[0]);
        if(decoded > chunk || input > data + size) {
            abort();
        }

        for(size_t i = 0; i < decoded; i++) {
            if(codepoints[i] > PASSGEN_UNICODE_MAX) {
                abort();
            }

            // the decoder only produces codepoints that can be encoded, and
            // encoding them cannot take more space than they took before.
            int bytes = passgen_utf8_encode_codepoint(
                &encoded[encoded_len],
                codepoints[i]);
            if(bytes < 1 || bytes > 4 || bytes != widths[i]) {
                abort();
            }
            encoded_len += (size_t) bytes;
            if(encoded_len > size) {
                abort();
            }
        }

        // no progress with more input to go would loop forever.
        if(ret > PASSGEN_UTF8_SUCCESS && decoded == 0) {
            abort();
        }
    } while(ret > PASSGEN_UTF8_SUCCESS);

    // what was decoded is exactly what was consumed.
    if(encoded_len > (size_t) (input - data) ||
       0 != memcmp(encoded, data, encoded_len)) {
        abort();
    }

    // if all of it was valid, all of it was consumed.
    if(ret == PASSGEN_UTF8_SUCCESS && encoded_len != size) {
        abort();
    }

    free(encoded);

    return 0;
}
