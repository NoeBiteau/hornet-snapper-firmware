#ifndef HS_PROTO_UART_FRAME_H
#define HS_PROTO_UART_FRAME_H

#include <stddef.h>
#include <stdint.h>

#define HS_UART_MAGIC0 0xAAu
#define HS_UART_MAGIC1 0x55u
#define HS_UART_MAX_PAYLOAD 240u
/* Frame layout:
 * [0xAA][0x55][LEN_LO][LEN_HI][TYPE][SEQ][PAYLOAD...][CRC_LO][CRC_HI]
 * LEN counts PAYLOAD bytes only (not header, not CRC).
 * CRC16-CCITT-FALSE over [LEN_LO..PAYLOAD_END].
 */
#define HS_UART_HEADER_BYTES 6u
#define HS_UART_TRAILER_BYTES 2u
#define HS_UART_OVERHEAD_BYTES (HS_UART_HEADER_BYTES + HS_UART_TRAILER_BYTES)

/* Wire message type IDs. Stable; do not renumber. */
typedef enum {
    HS_UART_MSG_CONFIG_PUSH  = 0x01,
    HS_UART_MSG_EVENT        = 0x02,
    HS_UART_MSG_TELEM        = 0x03,
    HS_UART_MSG_OTA_CHUNK    = 0x04,
    HS_UART_MSG_SHUTDOWN     = 0x05,
    HS_UART_MSG_LOG          = 0x06,
    HS_UART_MSG_TILT_TARGET  = 0x07,
} hs_uart_msg_t;

typedef struct {
    hs_uart_msg_t type;
    uint8_t       seq;
    uint16_t      payload_len;
    const uint8_t *payload;
} hs_uart_frame_t;

/* Returns total bytes written on success (> 0), 0 if buffer too small,
 * -1 if payload_len > HS_UART_MAX_PAYLOAD. Buffer must be at least
 * payload_len + HS_UART_OVERHEAD_BYTES. */
int hs_uart_encode(const hs_uart_frame_t *f, uint8_t *out, size_t out_cap);

/* Returns total bytes consumed on success (> 0), 0 on incomplete buffer,
 * -1 on framing error, -2 on CRC mismatch. On success, *out is populated
 * and out->payload points into the input buffer (no copy). */
int hs_uart_decode(const uint8_t *in, size_t in_len, hs_uart_frame_t *out);

#endif
