#include <stdio.h>
#include <string.h>
#include "uart_frame.h"
#include "tilt_target.h"
#include "event.h"
#include "version.h"

static void print_hex(const uint8_t *b, size_t n) {
    for (size_t i = 0; i < n; i++) printf("%02X", b[i]);
}

static void emit_uart_frame(const char *name, hs_uart_msg_t type, uint8_t seq,
                            const uint8_t *payload, uint16_t plen, int last) {
    hs_uart_frame_t f = {.type = type, .seq = seq, .payload_len = plen, .payload = payload};
    uint8_t buf[256];
    int n = hs_uart_encode(&f, buf, sizeof(buf));

    printf("    {\n");
    printf("      \"name\": \"%s\",\n", name);
    printf("      \"type\": %u,\n", (unsigned)type);
    printf("      \"seq\": %u,\n", seq);
    printf("      \"payload_hex\": \"");
    print_hex(payload, plen);
    printf("\",\n");
    printf("      \"wire_hex\": \"");
    print_hex(buf, (size_t)n);
    printf("\"\n");
    printf("    }%s\n", last ? "" : ",");
}

int main(void) {
    printf("{\n");
    printf("  \"proto_version\": {\"major\": %d, \"minor\": %d},\n",
           HS_PROTO_VERSION_MAJOR, HS_PROTO_VERSION_MINOR);
    printf("  \"uart_frames\": [\n");

    /* Vector 1: empty payload */
    emit_uart_frame("empty", HS_UART_MSG_LOG, 0x00, NULL, 0, 0);

    /* Vector 2: tiny event */
    {
        hs_event_t e = {
            .timestamp = 1746543791u,
            .type = HS_EVT_TYPE_DETECT,
            .class_id = 1, .confidence = 91, .fired = 1,
            .bbox = {500, 400, 50, 60}, .flags = 0, .battery_mv = 3782,
        };
        uint8_t pl[HS_EVENT_WIRE_BYTES];
        hs_event_encode(&e, pl);
        emit_uart_frame("event_detect_v1", HS_UART_MSG_EVENT, 0x42,
                        pl, HS_EVENT_WIRE_BYTES, 0);
    }

    /* Vector 3: tilt target */
    {
        hs_tilt_target_t t = {
            .bbox_y = 540, .bbox_y_center = 540,
            .velocity_y = -12, .confidence = 88,
            .flags = HS_TILT_FLAG_NEW_TARGET,
        };
        uint8_t pl[HS_TILT_TARGET_WIRE_BYTES];
        hs_tilt_target_encode(&t, pl);
        emit_uart_frame("tilt_target_centered", HS_UART_MSG_TILT_TARGET, 0x07,
                        pl, HS_TILT_TARGET_WIRE_BYTES, 1);
    }

    printf("  ]\n}\n");
    return 0;
}
