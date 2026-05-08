#include "app_context.h"
#include "fsm/state_machine.h"

extern "C" {
#include "event.h"
#include "uart_frame.h"
}

#include <cassert>
#include <cstring>

static uint16_t le16(const uint8_t* p) {
    return static_cast<uint16_t>(p[0] | (static_cast<uint16_t>(p[1]) << 8));
}

static uint32_t le32(const uint8_t* p) {
    return static_cast<uint32_t>(p[0]) |
           (static_cast<uint32_t>(p[1]) << 8) |
           (static_cast<uint32_t>(p[2]) << 16) |
           (static_cast<uint32_t>(p[3]) << 24);
}

static hs::c3::StateMachineInput input_with_boot_complete() {
    hs::c3::StateMachineInput in{};
    in.boot_complete = true;
    return in;
}

static hs::c3::StateMachineInput input_with_init_complete() {
    hs::c3::StateMachineInput in{};
    in.init_complete = true;
    return in;
}

static hs::c3::StateMachineInput input_with_selftest_pass() {
    hs::c3::StateMachineInput in{};
    in.selftest_pass = true;
    return in;
}

static hs::c3::StateMachineInput input_with_arm_requested() {
    hs::c3::StateMachineInput in{};
    in.arm_requested = true;
    return in;
}

static hs::c3::StateMachineInput input_with_target_detected() {
    hs::c3::StateMachineInput in{};
    in.target_detected = true;
    return in;
}

static hs::c3::StateMachineInput input_with_tilt_settled() {
    hs::c3::StateMachineInput in{};
    in.tilt_settled = true;
    return in;
}

int main() {
    using namespace hs::c3;

    AppContext::Options options = AppContext::Options::memory_only();
    options.fixed_unix_time = 4321;
    AppContext ctx(options);
    StateMachine fsm(ctx);
    fsm.tick(input_with_boot_complete());
    fsm.tick(input_with_init_complete());
    fsm.tick(input_with_selftest_pass());
    fsm.tick(input_with_arm_requested());
    fsm.tick({});
    fsm.tick(input_with_target_detected());
    fsm.tick(input_with_tilt_settled());

    VisionFireEvent fire{};
    fire.timestamp = 1234;
    fire.class_id = 1;
    fire.confidence = 88;
    fire.bbox[0] = 101;
    fire.bbox[1] = 202;
    fire.bbox[2] = 303;
    fire.bbox[3] = 404;
    fsm.inject_fire_irq(fire);

    assert(ctx.uart_frames().size() == 1);
    hs_uart_frame_t frame{};
    int consumed = hs_uart_decode(ctx.uart_frames()[0].data(), ctx.uart_frames()[0].size(), &frame);
    assert(consumed == static_cast<int>(ctx.uart_frames()[0].size()));
    assert(frame.type == HS_UART_MSG_EVENT);
    assert(frame.payload_len == HS_EVENT_WIRE_BYTES);
    assert(le32(frame.payload) == 4321);
    assert(frame.payload[4] == HS_EVT_TYPE_FIRE);
    assert(frame.payload[5] == 1);
    assert(frame.payload[6] == 88);
    assert(frame.payload[7] == 1);
    assert(le16(frame.payload + 8) == 101);
    assert(le16(frame.payload + 10) == 202);
    assert(le16(frame.payload + 12) == 303);
    assert(le16(frame.payload + 14) == 404);

    auto events = ctx.storage().load_all();
    assert(events.size() == 1);
    assert(events[0].timestamp == 4321);
    assert(events[0].type == HS_EVT_TYPE_FIRE);
    assert(events[0].fired == 1);
    assert(ctx.log_lines().size() == 1);
    assert(ctx.log_lines()[0].find("fire event") != std::string::npos);
    return 0;
}
