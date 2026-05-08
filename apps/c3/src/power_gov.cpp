#include "power_gov.h"

namespace hs::c3 {

void PowerGovernor::set_rv_power(bool enabled) {
    rv_power_enabled_ = enabled;
    record("rv_power", enabled);
}

void PowerGovernor::set_armed(bool armed) {
    armed_ = armed;
    record("armed", armed);
}

void PowerGovernor::set_vision_enabled(bool enabled) {
    vision_enabled_ = enabled;
    record("vision", enabled);
}

void PowerGovernor::request_sleep() {
    sleep_requested_ = true;
    actions_.push_back("sleep");
}

void PowerGovernor::record(const char* action, bool enabled) {
    std::string out(action);
    out += enabled ? ":on" : ":off";
    actions_.push_back(out);
}

}  // namespace hs::c3
