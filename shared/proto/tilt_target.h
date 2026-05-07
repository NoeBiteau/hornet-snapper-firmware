#ifndef HS_PROTO_TILT_TARGET_H
#define HS_PROTO_TILT_TARGET_H

#include <stdint.h>

/* Wire layout, little-endian, packed (10 bytes total):
 *   bbox_y        : u16
 *   bbox_y_center : u16
 *   velocity_y    : i16  (pixels per second)
 *   confidence    : u8   (0-100)
 *   flags         : u8   (bit0 = new_target, bit1 = lost)
 *   reserved      : u16  (must be 0)
 */
#define HS_TILT_TARGET_WIRE_BYTES 10u
#define HS_TILT_FLAG_NEW_TARGET 0x01u
#define HS_TILT_FLAG_LOST       0x02u

typedef struct {
    uint16_t bbox_y;
    uint16_t bbox_y_center;
    int16_t  velocity_y;
    uint8_t  confidence;
    uint8_t  flags;
} hs_tilt_target_t;

/* Encode into wire buffer (caller-provided, must be >= HS_TILT_TARGET_WIRE_BYTES).
 * Returns number of bytes written (always HS_TILT_TARGET_WIRE_BYTES). */
static inline uint8_t hs_tilt_target_encode(const hs_tilt_target_t *t, uint8_t *out) {
    out[0] = (uint8_t)(t->bbox_y & 0xFFu);
    out[1] = (uint8_t)((t->bbox_y >> 8) & 0xFFu);
    out[2] = (uint8_t)(t->bbox_y_center & 0xFFu);
    out[3] = (uint8_t)((t->bbox_y_center >> 8) & 0xFFu);
    out[4] = (uint8_t)((uint16_t)t->velocity_y & 0xFFu);
    out[5] = (uint8_t)(((uint16_t)t->velocity_y >> 8) & 0xFFu);
    out[6] = t->confidence;
    out[7] = t->flags;
    out[8] = 0; out[9] = 0;
    return HS_TILT_TARGET_WIRE_BYTES;
}

/* Returns 1 on success, 0 if in_len < HS_TILT_TARGET_WIRE_BYTES. */
static inline int hs_tilt_target_decode(const uint8_t *in, uint16_t in_len, hs_tilt_target_t *out) {
    if (in_len < HS_TILT_TARGET_WIRE_BYTES) return 0;
    out->bbox_y        = (uint16_t)(in[0] | ((uint16_t)in[1] << 8));
    out->bbox_y_center = (uint16_t)(in[2] | ((uint16_t)in[3] << 8));
    out->velocity_y    = (int16_t)(in[4] | ((uint16_t)in[5] << 8));
    out->confidence    = in[6];
    out->flags         = in[7];
    return 1;
}

#endif
