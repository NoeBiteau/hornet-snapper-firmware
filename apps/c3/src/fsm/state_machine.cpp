#include "fsm/state_machine.h"

namespace hs::c3 {

void StateMachine::tick(const StateMachineInput& input) {
    if (consume_injected_fault()) {
        return;
    }

    switch (state_) {
        case C3State::Boot:
            if (input.boot_complete) enter(C3State::Init);
            break;
        case C3State::Init:
            if (input.init_complete) enter(C3State::SelfTest);
            break;
        case C3State::SelfTest:
            if (input.selftest_pass) {
                enter(C3State::Idle);
            } else {
                inject_fault(FaultId::F12_StuckInFault);
            }
            break;
        case C3State::Idle:
            if (input.sleep_requested) {
                enter(C3State::Sleep);
            } else if (input.arm_requested) {
                enter(C3State::Arm);
            }
            break;
        case C3State::Arm:
            enter(C3State::HuntScan);
            break;
        case C3State::HuntScan:
            if (input.sleep_requested || input.hunt_timeout) {
                enter(C3State::Sleep);
            } else if (input.target_detected) {
                enter(C3State::HuntTrack);
            }
            break;
        case C3State::HuntTrack:
            if (input.sleep_requested || input.hunt_timeout) {
                enter(C3State::Sleep);
            } else if (input.tilt_settled) {
                enter(C3State::HuntLocked);
            }
            break;
        case C3State::HuntLocked:
            if (input.sleep_requested || input.hunt_timeout) {
                enter(C3State::Sleep);
            } else if (input.target_lost || input.tilt_unsettled) {
                enter(C3State::HuntTrack);
            }
            break;
        case C3State::Fire:
            enter(C3State::Rearm);
            break;
        case C3State::Rearm:
            if (input.sleep_requested) {
                enter(C3State::Sleep);
            } else if (input.rearm_complete) {
                enter(C3State::HuntScan);
            }
            break;
        case C3State::Sleep:
            if (input.arm_requested) enter(C3State::Arm);
            break;
        case C3State::Fault:
            if (input.recover_requested) enter(C3State::Recover);
            break;
        case C3State::Recover:
            enter(C3State::Idle);
            break;
    }
}

void StateMachine::inject_fire_irq(const VisionFireEvent& event) {
    if (state_ != C3State::HuntLocked) {
        ctx_.log("ignored FIRE_IRQ state=" + std::string(state_name(state_)));
        return;
    }
    ctx_.emit_event(make_fire_event(event));
    enter(C3State::Fire);
}

void StateMachine::inject_fault(FaultId id) {
    FaultContext fault_context{};
    if (id == FaultId::F1_MotorStall || id == FaultId::F16_TiltStall) {
        fault_context.retry_count = 3;
    }
    auto disposition = evaluate_fault(id, fault_context);
    ctx_.event_bus().publish(BusEvent::fault(id));
    ctx_.emit_event(make_fault_event(id, disposition));
    if (disposition.action == FaultAction::PowerCycleRv) {
        ctx_.power().set_rv_power(false);
        ctx_.power().set_rv_power(true);
    }
    if (disposition.action == FaultAction::DeepSleep) {
        ctx_.power().request_sleep();
    }
    if (disposition.terminal) {
        enter(C3State::Fault);
    }
}

bool StateMachine::consume_injected_fault() {
    for (int i = 1; i <= 17; ++i) {
        auto id = static_cast<FaultId>(i);
        if (ctx_.fault_injector().is_active(id)) {
            ctx_.fault_injector().set(id, false);
            inject_fault(id);
            return true;
        }
    }
    return false;
}

void StateMachine::enter(C3State state) {
    state_ = state;
    switch (state_) {
        case C3State::HuntScan:
            hunt_substate_ = HuntSubstate::Scan;
            ctx_.power().set_rv_power(true);
            ctx_.power().set_vision_enabled(true);
            break;
        case C3State::HuntTrack:
            hunt_substate_ = HuntSubstate::Track;
            break;
        case C3State::HuntLocked:
            hunt_substate_ = HuntSubstate::Locked;
            break;
        case C3State::Arm:
            hunt_substate_ = HuntSubstate::None;
            ctx_.power().set_armed(true);
            ctx_.power().set_rv_power(true);
            ctx_.power().set_vision_enabled(true);
            break;
        case C3State::Sleep:
            hunt_substate_ = HuntSubstate::None;
            ctx_.power().set_vision_enabled(false);
            ctx_.power().set_rv_power(false);
            ctx_.power().set_armed(false);
            ctx_.power().request_sleep();
            break;
        default:
            hunt_substate_ = HuntSubstate::None;
            break;
    }
}

hs_event_t StateMachine::make_fire_event(const VisionFireEvent& event) const {
    hs_event_t out{};
    out.timestamp = ctx_.now_unix();
    out.type = HS_EVT_TYPE_FIRE;
    out.class_id = event.class_id;
    out.confidence = event.confidence;
    out.fired = 1;
    for (int i = 0; i < 4; ++i) {
        out.bbox[i] = event.bbox[i];
    }
    out.battery_mv = ctx_.battery_mv();
    return out;
}

hs_event_t StateMachine::make_fault_event(FaultId id, const FaultDisposition& disposition) const {
    hs_event_t out{};
    out.type = HS_EVT_TYPE_FAULT;
    out.flags = disposition.event_flags;
    out.class_id = static_cast<uint8_t>(id);
    out.battery_mv = ctx_.battery_mv();
    return out;
}

const char* state_name(C3State state) {
    switch (state) {
        case C3State::Boot: return "BOOT";
        case C3State::Init: return "INIT";
        case C3State::SelfTest: return "SELFTEST";
        case C3State::Idle: return "IDLE";
        case C3State::Arm: return "ARM";
        case C3State::HuntScan: return "HUNT_SCAN";
        case C3State::HuntTrack: return "HUNT_TRACK";
        case C3State::HuntLocked: return "HUNT_LOCKED";
        case C3State::Fire: return "FIRE";
        case C3State::Rearm: return "REARM";
        case C3State::Sleep: return "SLEEP";
        case C3State::Fault: return "FAULT";
        case C3State::Recover: return "RECOVER";
    }
    return "UNKNOWN";
}

const char* hunt_substate_name(HuntSubstate state) {
    switch (state) {
        case HuntSubstate::None: return "NONE";
        case HuntSubstate::Scan: return "HUNT_SCAN";
        case HuntSubstate::Track: return "HUNT_TRACK";
        case HuntSubstate::Locked: return "HUNT_LOCKED";
    }
    return "UNKNOWN";
}

}  // namespace hs::c3
