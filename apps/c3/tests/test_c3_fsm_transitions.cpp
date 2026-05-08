#include "app_context.h"
#include "fsm/state_machine.h"

#include <cassert>

static hs::c3::StateMachineInput input_boot_complete() {
    hs::c3::StateMachineInput in{};
    in.boot_complete = true;
    return in;
}

static hs::c3::StateMachineInput input_init_complete() {
    hs::c3::StateMachineInput in{};
    in.init_complete = true;
    return in;
}

static hs::c3::StateMachineInput input_selftest_pass() {
    hs::c3::StateMachineInput in{};
    in.selftest_pass = true;
    return in;
}

static hs::c3::StateMachineInput input_arm_requested() {
    hs::c3::StateMachineInput in{};
    in.arm_requested = true;
    return in;
}

static hs::c3::StateMachineInput input_target_detected() {
    hs::c3::StateMachineInput in{};
    in.target_detected = true;
    return in;
}

static hs::c3::StateMachineInput input_tilt_settled() {
    hs::c3::StateMachineInput in{};
    in.tilt_settled = true;
    return in;
}

static hs::c3::StateMachineInput input_target_lost() {
    hs::c3::StateMachineInput in{};
    in.target_lost = true;
    return in;
}

static hs::c3::StateMachineInput input_tilt_unsettled() {
    hs::c3::StateMachineInput in{};
    in.tilt_unsettled = true;
    return in;
}

static hs::c3::StateMachineInput input_rearm_complete() {
    hs::c3::StateMachineInput in{};
    in.rearm_complete = true;
    return in;
}

static hs::c3::StateMachineInput input_sleep_requested() {
    hs::c3::StateMachineInput in{};
    in.sleep_requested = true;
    return in;
}

static hs::c3::VisionFireEvent fire_event() {
    hs::c3::VisionFireEvent fire{};
    fire.timestamp = 100;
    fire.class_id = 1;
    fire.confidence = 93;
    fire.bbox[0] = 10;
    fire.bbox[1] = 20;
    fire.bbox[2] = 30;
    fire.bbox[3] = 40;
    return fire;
}

int main() {
    using namespace hs::c3;

    AppContext ctx(AppContext::Options::memory_only());
    StateMachine fsm(ctx);

    assert(fsm.state() == C3State::Boot);
    fsm.tick(input_boot_complete());
    assert(fsm.state() == C3State::Init);
    fsm.tick(input_init_complete());
    assert(fsm.state() == C3State::SelfTest);
    fsm.tick(input_selftest_pass());
    assert(fsm.state() == C3State::Idle);
    fsm.tick(input_arm_requested());
    assert(fsm.state() == C3State::Arm);
    fsm.tick({});
    assert(fsm.state() == C3State::HuntScan);
    assert(fsm.hunt_substate() == HuntSubstate::Scan);
    fsm.tick(input_target_detected());
    assert(fsm.state() == C3State::HuntTrack);
    assert(fsm.hunt_substate() == HuntSubstate::Track);
    fsm.tick(input_tilt_settled());
    assert(fsm.state() == C3State::HuntLocked);
    assert(fsm.hunt_substate() == HuntSubstate::Locked);

    fsm.tick(input_target_lost());
    assert(fsm.state() == C3State::HuntTrack);
    assert(fsm.hunt_substate() == HuntSubstate::Track);
    fsm.tick(input_tilt_settled());
    assert(fsm.state() == C3State::HuntLocked);
    fsm.tick(input_tilt_unsettled());
    assert(fsm.state() == C3State::HuntTrack);
    assert(fsm.hunt_substate() == HuntSubstate::Track);
    fsm.tick(input_tilt_settled());
    assert(fsm.state() == C3State::HuntLocked);

    fsm.inject_fire_irq(fire_event());
    assert(fsm.state() == C3State::Fire);
    fsm.tick({});
    assert(fsm.state() == C3State::Rearm);
    fsm.tick(input_rearm_complete());
    assert(fsm.state() == C3State::HuntScan);
    fsm.tick(input_sleep_requested());
    assert(fsm.state() == C3State::Sleep);

    assert(!ctx.power().rv_power_enabled());
    assert(!ctx.power().vision_enabled());
    assert(!ctx.power().armed());
    assert(ctx.power().sleep_requested());
    return 0;
}
