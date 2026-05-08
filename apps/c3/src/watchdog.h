#ifndef HS_C3_WATCHDOG_H
#define HS_C3_WATCHDOG_H

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace hs::c3 {

enum class WatchdogAction {
    ResetC3,
    PowerCycleRv,
    ColdBoot,
};

class WatchdogSupervisor {
public:
    void register_task(std::string name, uint32_t timeout_ms, uint32_t now_ms);
    void record_task_heartbeat(const std::string& name, uint32_t now_ms);
    std::vector<WatchdogAction> check(uint32_t now_ms);

    void record_rv_alive(uint32_t now_ms);
    std::vector<WatchdogAction> note_rv_alive_missed();
    std::vector<WatchdogAction> note_brownout();

private:
    struct Task {
        uint32_t timeout_ms = 0;
        uint32_t last_heartbeat_ms = 0;
    };

    std::unordered_map<std::string, Task> tasks_;
    int rv_alive_miss_count_ = 0;
    uint32_t last_rv_alive_ms_ = 0;
};

}  // namespace hs::c3

#endif
