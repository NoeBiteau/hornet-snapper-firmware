#include "fault_tree.h"

namespace hs::c3 {

void hs_fault_inject_t::set(FaultId id, bool enabled) {
    active[static_cast<std::size_t>(id)] = enabled;
}

bool hs_fault_inject_t::is_active(FaultId id) const {
    return active[static_cast<std::size_t>(id)];
}

void hs_fault_inject_t::clear() {
    active.fill(false);
}

FaultDisposition evaluate_fault(FaultId id, FaultContext context) {
    FaultDisposition out;
    out.event_flags = static_cast<uint8_t>(id);
    switch (id) {
        case FaultId::F1_MotorStall:
            out.action = FaultAction::ReversePulseRetry;
            out.terminal = context.retry_count >= 3;
            break;
        case FaultId::F2_MotorOpen:
            out.action = FaultAction::MarkAndAlert;
            out.terminal = true;
            break;
        case FaultId::F3_LatchFailEngage:
            out.action = FaultAction::RetryCycle;
            out.terminal = context.retry_count >= 1;
            break;
        case FaultId::F4_SolenoidNoFire:
            out.action = FaultAction::MarkAndAlert;
            out.terminal = true;
            break;
        case FaultId::F5_Rv1106Hung:
            out.action = FaultAction::PowerCycleRv;
            break;
        case FaultId::F6_Rv1106BootFail:
            out.action = FaultAction::VisionDisabledFault;
            out.terminal = context.retry_count >= 3;
            out.degraded = !out.terminal;
            break;
        case FaultId::F7_CameraIspError:
            out.action = FaultAction::PowerCycleRv;
            break;
        case FaultId::F8_LoRaTxFail:
            out.action = FaultAction::BackoffStoreForward;
            break;
        case FaultId::F9_BleStackHang:
            out.action = FaultAction::ResetC3;
            break;
        case FaultId::F10_BatteryCritical:
            out.action = FaultAction::DeepSleep;
            out.terminal = true;
            break;
        case FaultId::F11_Brownout:
            out.action = FaultAction::ColdBoot;
            break;
        case FaultId::F12_StuckInFault:
            out.action = FaultAction::StayFault;
            out.terminal = context.recovery_attempts >= 24;
            break;
        case FaultId::F13_WatchdogReset:
            out.action = FaultAction::ResetC3;
            out.terminal = context.daily_reset_count > 5;
            break;
        case FaultId::F14_FilesystemCorrupt:
            out.action = FaultAction::ReformatFs;
            break;
        case FaultId::F15_CapChargeFail:
            out.action = FaultAction::MarkAndAlert;
            out.terminal = true;
            break;
        case FaultId::F16_TiltStall:
            out.action = FaultAction::ReversePulseRetry;
            out.terminal = context.retry_count >= 3;
            break;
        case FaultId::F17_TiltEncoderFail:
            out.action = FaultAction::DisableTiltFallback;
            out.degraded = true;
            break;
    }
    return out;
}

const char* fault_name(FaultId id) {
    switch (id) {
        case FaultId::F1_MotorStall: return "F-1 motor stall";
        case FaultId::F2_MotorOpen: return "F-2 motor open";
        case FaultId::F3_LatchFailEngage: return "F-3 latch fail engage";
        case FaultId::F4_SolenoidNoFire: return "F-4 solenoid no-fire";
        case FaultId::F5_Rv1106Hung: return "F-5 RV1106 hung";
        case FaultId::F6_Rv1106BootFail: return "F-6 RV1106 boot fail";
        case FaultId::F7_CameraIspError: return "F-7 camera ISP error";
        case FaultId::F8_LoRaTxFail: return "F-8 LoRa Tx fail";
        case FaultId::F9_BleStackHang: return "F-9 BLE stack hang";
        case FaultId::F10_BatteryCritical: return "F-10 battery critical";
        case FaultId::F11_Brownout: return "F-11 brownout";
        case FaultId::F12_StuckInFault: return "F-12 stuck in FAULT";
        case FaultId::F13_WatchdogReset: return "F-13 watchdog reset";
        case FaultId::F14_FilesystemCorrupt: return "F-14 filesystem corrupt";
        case FaultId::F15_CapChargeFail: return "F-15 cap charge fail";
        case FaultId::F16_TiltStall: return "F-16 tilt stall";
        case FaultId::F17_TiltEncoderFail: return "F-17 tilt encoder fail";
    }
    return "F-? unknown";
}

}  // namespace hs::c3
