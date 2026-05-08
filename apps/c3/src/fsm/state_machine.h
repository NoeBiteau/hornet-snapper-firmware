#ifndef HS_C3_STATE_MACHINE_H
#define HS_C3_STATE_MACHINE_H

#include "app_context.h"

#include <cstdint>

namespace hs::c3 {

enum class C3State {
    Boot,
    Init,
    SelfTest,
    Idle,
    Arm,
    HuntScan,
    HuntTrack,
    HuntLocked,
    Fire,
    Rearm,
    Sleep,
    Fault,
    Recover,
};

enum class HuntSubstate {
    None,
    Scan,
    Track,
    Locked,
};

struct StateMachineInput {
    bool boot_complete = false;
    bool init_complete = false;
    bool selftest_pass = true;
    bool arm_requested = false;
    bool sleep_requested = false;
    bool hunt_timeout = false;
    bool target_detected = false;
    bool target_lost = false;
    bool tilt_settled = false;
    bool tilt_unsettled = false;
    bool rearm_complete = false;
    bool recover_requested = false;
};

struct VisionFireEvent {
    uint32_t timestamp = 0;
    uint8_t class_id = 1;
    uint8_t confidence = 90;
    uint16_t bbox[4] = {0, 0, 100, 100};
};

class StateMachine {
public:
    explicit StateMachine(AppContext& ctx) : ctx_(ctx) {}

    void tick(const StateMachineInput& input);
    void inject_fire_irq(const VisionFireEvent& event);
    void inject_fault(FaultId id);

    C3State state() const { return state_; }
    HuntSubstate hunt_substate() const { return hunt_substate_; }

private:
    bool consume_injected_fault();
    void enter(C3State state);
    hs_event_t make_fire_event(const VisionFireEvent& event) const;
    hs_event_t make_fault_event(FaultId id, const FaultDisposition& disposition) const;

    AppContext& ctx_;
    C3State state_ = C3State::Boot;
    HuntSubstate hunt_substate_ = HuntSubstate::None;
};

const char* state_name(C3State state);
const char* hunt_substate_name(HuntSubstate state);

}  // namespace hs::c3

#endif
