#include "app_context.h"
#include "fsm/state_machine.h"

#include <cassert>

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

static hs::c3::VisionFireEvent fire_event() {
    hs::c3::VisionFireEvent fire{};
    fire.timestamp = 50;
    fire.class_id = 1;
    fire.confidence = 90;
    fire.bbox[0] = 1;
    fire.bbox[1] = 2;
    fire.bbox[2] = 3;
    fire.bbox[3] = 4;
    return fire;
}

int main() {
    using namespace hs::c3;

    AppContext ctx(AppContext::Options::memory_only());
    StateMachine fsm(ctx);
    fsm.tick(input_with_boot_complete());
    fsm.tick(input_with_init_complete());
    fsm.tick(input_with_selftest_pass());
    fsm.tick(input_with_arm_requested());
    fsm.tick({});
    assert(fsm.state() == C3State::HuntScan);

    VisionFireEvent fire = fire_event();
    fsm.inject_fire_irq(fire);
    assert(fsm.state() == C3State::HuntScan);
    assert(ctx.uart_frames().empty());
    assert(ctx.storage().size() == 0);
    assert(ctx.log_lines().back().find("ignored FIRE_IRQ") != std::string::npos);

    fsm.tick(input_with_target_detected());
    assert(fsm.state() == C3State::HuntTrack);
    fsm.inject_fire_irq(fire);
    assert(fsm.state() == C3State::HuntTrack);
    assert(ctx.uart_frames().empty());
    assert(ctx.storage().size() == 0);
    assert(ctx.log_lines().back().find("ignored FIRE_IRQ") != std::string::npos);
    return 0;
}
