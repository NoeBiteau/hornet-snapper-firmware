#include <string.h>
#include "uart_frame.h"
#include "crc16.h"

int hs_uart_encode(const hs_uart_frame_t *f, uint8_t *out, size_t out_cap) {
    if (f->payload_len > HS_UART_MAX_PAYLOAD) return -1;
    size_t total = (size_t)f->payload_len + HS_UART_OVERHEAD_BYTES;
    if (out_cap < total) return 0;

    out[0] = HS_UART_MAGIC0;
    out[1] = HS_UART_MAGIC1;
    out[2] = (uint8_t)(f->payload_len & 0xFFu);
    out[3] = (uint8_t)((f->payload_len >> 8) & 0xFFu);
    out[4] = (uint8_t)f->type;
    out[5] = f->seq;
    if (f->payload_len > 0 && f->payload != NULL) {
        memcpy(out + HS_UART_HEADER_BYTES, f->payload, f->payload_len);
    }
    /* CRC over [LEN_LO .. last payload byte], excludes magic and CRC itself. */
    uint16_t crc = hs_crc16_ccitt(out + 2, f->payload_len + 4u);
    out[HS_UART_HEADER_BYTES + f->payload_len + 0] = (uint8_t)(crc & 0xFFu);
    out[HS_UART_HEADER_BYTES + f->payload_len + 1] = (uint8_t)((crc >> 8) & 0xFFu);
    return (int)total;
}

int hs_uart_decode(const uint8_t *in, size_t in_len, hs_uart_frame_t *out) {
    if (in_len < HS_UART_OVERHEAD_BYTES) return 0;
    if (in[0] != HS_UART_MAGIC0 || in[1] != HS_UART_MAGIC1) return -1;
    uint16_t plen = (uint16_t)(in[2] | ((uint16_t)in[3] << 8));
    if (plen > HS_UART_MAX_PAYLOAD) return -1;
    size_t total = (size_t)plen + HS_UART_OVERHEAD_BYTES;
    if (in_len < total) return 0;

    uint16_t crc_calc = hs_crc16_ccitt(in + 2, plen + 4u);
    uint16_t crc_wire = (uint16_t)(in[HS_UART_HEADER_BYTES + plen]
                       | ((uint16_t)in[HS_UART_HEADER_BYTES + plen + 1] << 8));
    if (crc_calc != crc_wire) return -2;

    out->type = (hs_uart_msg_t)in[4];
    out->seq = in[5];
    out->payload_len = plen;
    out->payload = (plen > 0) ? (in + HS_UART_HEADER_BYTES) : NULL;
    return (int)total;
}
