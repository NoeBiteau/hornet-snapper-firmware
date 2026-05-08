#ifndef HS_C3_POWER_GOV_H
#define HS_C3_POWER_GOV_H

#include <string>
#include <vector>

namespace hs::c3 {

class PowerGovernor {
public:
    void set_rv_power(bool enabled);
    void set_armed(bool armed);
    void set_vision_enabled(bool enabled);
    void request_sleep();

    bool rv_power_enabled() const { return rv_power_enabled_; }
    bool armed() const { return armed_; }
    bool vision_enabled() const { return vision_enabled_; }
    bool sleep_requested() const { return sleep_requested_; }
    const std::vector<std::string>& actions() const { return actions_; }

private:
    void record(const char* action, bool enabled);

    bool rv_power_enabled_ = false;
    bool armed_ = false;
    bool vision_enabled_ = false;
    bool sleep_requested_ = false;
    std::vector<std::string> actions_;
};

}  // namespace hs::c3

#endif
