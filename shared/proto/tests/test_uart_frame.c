#include <assert.h>
#include <string.h>
#include "../uart_frame.h"

static void test_encode_simple(void) {
    uint8_t payload[] = {0x10, 0x20, 0x30};
    hs_uart_frame_t f = {
        .type = HS_UART_MSG_EVENT,
        .seq = 0x42,
        .payload_len = 3,
        .payload = payload,
    };
    uint8_t buf[64];
    int n = hs_uart_encode(&f, buf, sizeof(buf));
    assert(n == 11);  /* 6 header + 3 payload + 2 crc */
    assert(buf[0] == 0xAA);
    assert(buf[1] == 0x55);
    assert(buf[2] == 0x03 && buf[3] == 0x00);  /* LEN little-endian = 3 */
    assert(buf[4] == 0x02);                    /* TYPE = EVENT */
    assert(buf[5] == 0x42);                    /* SEQ */
    assert(buf[6] == 0x10 && buf[7] == 0x20 && buf[8] == 0x30);
    /* CRC verified by round-trip below. */
}

static void test_roundtrip(void) {
    uint8_t payload[] = {1, 2, 3, 4, 5};
    hs_uart_frame_t in = {
        .type = HS_UART_MSG_TILT_TARGET,
        .seq = 0x07,
        .payload_len = 5,
        .payload = payload,
    };
    uint8_t buf[64];
    int n = hs_uart_encode(&in, buf, sizeof(buf));
    assert(n > 0);

    hs_uart_frame_t out = {0};
    int m = hs_uart_decode(buf, (size_t)n, &out);
    assert(m == n);
    assert(out.type == in.type);
    assert(out.seq == in.seq);
    assert(out.payload_len == in.payload_len);
    assert(memcmp(out.payload, in.payload, in.payload_len) == 0);
}

static void test_bad_crc(void) {
    uint8_t payload[] = {1, 2, 3};
    hs_uart_frame_t in = {.type = HS_UART_MSG_EVENT, .seq = 1, .payload_len = 3, .payload = payload};
    uint8_t buf[64];
    int n = hs_uart_encode(&in, buf, sizeof(buf));
    assert(n > 0);
    buf[n - 1] ^= 0xFF;  /* corrupt CRC high byte */
    hs_uart_frame_t out = {0};
    int m = hs_uart_decode(buf, (size_t)n, &out);
    assert(m == -2);  /* CRC mismatch */
}

static void test_buffer_too_small(void) {
    uint8_t payload[] = {1};
    hs_uart_frame_t in = {.type = HS_UART_MSG_EVENT, .seq = 1, .payload_len = 1, .payload = payload};
    uint8_t buf[5];  /* needs 9, capacity 5 */
    int n = hs_uart_encode(&in, buf, sizeof(buf));
    assert(n == 0);
}

int main(void) {
    test_encode_simple();
    test_roundtrip();
    test_bad_crc();
    test_buffer_too_small();
    return 0;
}
