#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../uart_frame.h"

/* Naive JSON walker: finds N occurrences of "wire_hex": "..." and "type": N
 * and "seq": N in document order. Sufficient for our generated file shape. */

static long file_size(FILE *f) {
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    return n;
}

static int hex_nibble(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

static size_t hex_to_bytes(const char *hex, uint8_t *out, size_t cap) {
    size_t n = 0;
    while (hex[0] && hex[1] && n < cap) {
        int hi = hex_nibble(hex[0]);
        int lo = hex_nibble(hex[1]);
        if (hi < 0 || lo < 0) break;
        out[n++] = (uint8_t)((hi << 4) | lo);
        hex += 2;
    }
    return n;
}

int main(int argc, char **argv) {
    const char *path = (argc >= 2) ? argv[1]
                       : "../shared/proto/vectors/v1/uart_frame.json";
    FILE *f = fopen(path, "rb");
    if (!f) { perror(path); return 1; }
    long sz = file_size(f);
    char *doc = malloc((size_t)sz + 1);
    fread(doc, 1, (size_t)sz, f);
    doc[sz] = '\0';
    fclose(f);

    int count = 0;
    const char *cursor = doc;
    while (1) {
        const char *t = strstr(cursor, "\"type\":");
        if (!t) break;
        long type_val = strtol(t + 7, NULL, 10);

        const char *s = strstr(t, "\"seq\":");
        assert(s);
        long seq_val = strtol(s + 6, NULL, 10);

        const char *w = strstr(s, "\"wire_hex\":");
        assert(w);
        /* "wire_hex": is exactly 11 chars; skip past it then find opening
         * quote of the value, then find its matching closing quote. */
        const char *q1 = strchr(w + 11, '"');
        assert(q1);
        q1++;
        const char *q2 = strchr(q1, '"');
        assert(q2);

        uint8_t wire[256];
        size_t wlen = hex_to_bytes(q1, wire, sizeof(wire));
        assert(wlen >= HS_UART_OVERHEAD_BYTES);

        hs_uart_frame_t decoded = {0};
        int dn = hs_uart_decode(wire, wlen, &decoded);
        assert(dn > 0);
        assert((long)decoded.type == type_val);
        assert((long)decoded.seq == seq_val);

        uint8_t reenc[256];
        int en = hs_uart_encode(&decoded, reenc, sizeof(reenc));
        assert(en == (int)wlen);
        assert(memcmp(wire, reenc, (size_t)en) == 0);

        count++;
        cursor = q2 + 1;
    }

    free(doc);
    if (count < 1) {
        fprintf(stderr, "no vectors found in %s\n", path);
        return 2;
    }
    fprintf(stderr, "round-tripped %d vectors OK\n", count);
    return 0;
}
