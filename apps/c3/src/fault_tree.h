#ifndef HS_C3_FAULT_TREE_H
#define HS_C3_FAULT_TREE_H

#include <array>
#include <cstdint>

namespace hs::c3 {

enum class FaultId : uint8_t {
    F1_MotorStall = 1,
    F2_MotorOpen = 2,
    F3_LatchFailEngage = 3,
    F4_SolenoidNoFire = 4,
    F5_Rv1106Hung = 5,
    F6_Rv1106BootFail = 6,
    F7_CameraIspError = 7,
    F8_LoRaTxFail = 8,
    F9_BleStackHang = 9,
    F10_BatteryCritical = 10,
    F11_Brownout = 11,
    F12_StuckInFault = 12,
    F13_WatchdogReset = 13,
    F14_FilesystemCorrupt = 14,
    F15_CapChargeFail = 15,
    F16_TiltStall = 16,
    F17_TiltEncoderFail = 17,
};

enum class FaultAction : uint8_t {
    None,
    ReversePulseRetry,
    RetryCycle,
    MarkAndAlert,
    PowerCycleRv,
    VisionDisabledFault,
    BackoffStoreForward,
    ResetC3,
    DeepSleep,
    ColdBoot,
    StayFault,
    ReformatFs,
    DisableTiltFallback,
};

struct FaultContext {
    int retry_count = 0;
    int daily_reset_count = 0;
    int recovery_attempts = 0;
};

struct FaultDisposition {
    FaultAction action = FaultAction::None;
    bool terminal = false;
    bool degraded = false;
    uint8_t event_flags = 0;
};

struct hs_fault_inject_t {
    std::array<bool, 18> active{};

    void set(FaultId id, bool enabled = true);
    bool is_active(FaultId id) const;
    void clear();
};

FaultDisposition evaluate_fault(FaultId id, FaultContext context = {});
const char* fault_name(FaultId id);

}  // namespace hs::c3

#endif
