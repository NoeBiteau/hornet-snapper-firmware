#ifndef HS_PROTO_CRC16_H
#define HS_PROTO_CRC16_H

#include <stddef.h>
#include <stdint.h>

/* CRC-16/CCITT-FALSE: poly 0x1021, init 0xFFFF, no reflection, no final xor. */
uint16_t hs_crc16_ccitt(const uint8_t *data, size_t len);

#endif
