#ifndef HS_PROTO_EVENT_H
#define HS_PROTO_EVENT_H

#include <stdint.h>

/* Per control-spec section 11.1 — 24 bytes on the wire, little-endian. */
#define HS_EVENT_WIRE_BYTES 24u

typedef enum {
    HS_EVT_TYPE_BOOT     = 0,
    HS_EVT_TYPE_DETECT   = 1,
    HS_EVT_TYPE_FIRE     = 2,
    HS_EVT_TYPE_REARM    = 3,
    HS_EVT_TYPE_FAULT    = 4,
} hs_event_type_t;

typedef struct {
    uint32_t timestamp;     /* unix seconds */
    uint8_t  type;          /* hs_event_type_t */
    uint8_t  class_id;      /* 0=bee 1=velutina 2=crabro 3=other 4=bg */
    uint8_t  confidence;    /* 0-100 */
    uint8_t  fired;         /* 0|1 */
    uint16_t bbox[4];       /* x,y,w,h normalised to 0-1000 */
    uint8_t  flags;
    uint8_t  reserved;
    uint16_t battery_mv;
    uint16_t reserved2;
} hs_event_t;

static inline uint8_t hs_event_encode(const hs_event_t *e, uint8_t *out) {
    out[0] = (uint8_t)(e->timestamp & 0xFFu);
    out[1] = (uint8_t)((e->timestamp >> 8) & 0xFFu);
    out[2] = (uint8_t)((e->timestamp >> 16) & 0xFFu);
    out[3] = (uint8_t)((e->timestamp >> 24) & 0xFFu);
    out[4] = e->type;
    out[5] = e->class_id;
    out[6] = e->confidence;
    out[7] = e->fired;
    for (int i = 0; i < 4; i++) {
        out[8 + 2*i + 0] = (uint8_t)(e->bbox[i] & 0xFFu);
        out[8 + 2*i + 1] = (uint8_t)((e->bbox[i] >> 8) & 0xFFu);
    }
    out[16] = e->flags;
    out[17] = e->reserved;
    out[18] = (uint8_t)(e->battery_mv & 0xFFu);
    out[19] = (uint8_t)((e->battery_mv >> 8) & 0xFFu);
    out[20] = 0; out[21] = 0; out[22] = 0; out[23] = 0;
    return HS_EVENT_WIRE_BYTES;
}

#endif
