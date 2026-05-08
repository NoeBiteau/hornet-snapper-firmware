#include "app_context.h"
#include "fault_tree.h"
#include "fsm/state_machine.h"

#include <cassert>

int main() {
    using namespace hs::c3;

    for (int i = 1; i <= 17; ++i) {
        auto id = static_cast<FaultId>(i);
        FaultContext fault_ctx{};
        fault_ctx.retry_count = 3;
        auto d = evaluate_fault(id, fault_ctx);
        assert(fault_name(id)[0] == 'F');
        assert(d.action != FaultAction::None);
    }

    assert(evaluate_fault(FaultId::F5_Rv1106Hung, {}).terminal == false);
    assert(evaluate_fault(FaultId::F8_LoRaTxFail, {}).action == FaultAction::BackoffStoreForward);
    assert(evaluate_fault(FaultId::F10_BatteryCritical, {}).terminal == true);
    FaultContext tilt_retrying{};
    tilt_retrying.retry_count = 2;
    assert(evaluate_fault(FaultId::F16_TiltStall, tilt_retrying).terminal == false);
    FaultContext tilt_failed{};
    tilt_failed.retry_count = 3;
    assert(evaluate_fault(FaultId::F16_TiltStall, tilt_failed).terminal == true);
    FaultContext recoverable_stuck{};
    recoverable_stuck.recovery_attempts = 23;
    assert(evaluate_fault(FaultId::F12_StuckInFault, recoverable_stuck).terminal == false);
    FaultContext terminal_stuck{};
    terminal_stuck.recovery_attempts = 24;
    assert(evaluate_fault(FaultId::F12_StuckInFault, terminal_stuck).terminal == true);
    assert(evaluate_fault(FaultId::F17_TiltEncoderFail, {}).degraded == true);

    AppContext ctx(AppContext::Options::memory_only());
    StateMachine fsm(ctx);
    fsm.inject_fault(FaultId::F10_BatteryCritical);
    assert(fsm.state() == C3State::Fault);
    assert(ctx.storage().load_all().back().type == HS_EVT_TYPE_FAULT);

    AppContext injected_ctx(AppContext::Options::memory_only());
    StateMachine injected_fsm(injected_ctx);
    injected_ctx.fault_injector().set(FaultId::F2_MotorOpen);
    injected_fsm.tick({});
    assert(injected_fsm.state() == C3State::Fault);
    assert(!injected_ctx.fault_injector().is_active(FaultId::F2_MotorOpen));
    assert(injected_ctx.storage().load_all().back().class_id == static_cast<uint8_t>(FaultId::F2_MotorOpen));
    return 0;
}
