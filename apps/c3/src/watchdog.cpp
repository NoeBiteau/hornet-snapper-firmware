#include "watchdog.h"

namespace hs::c3 {

void WatchdogSupervisor::register_task(std::string name, uint32_t timeout_ms, uint32_t now_ms) {
    tasks_[std::move(name)] = Task{timeout_ms, now_ms};
}

void WatchdogSupervisor::record_task_heartbeat(const std::string& name, uint32_t now_ms) {
    auto it = tasks_.find(name);
    if (it != tasks_.end()) {
        it->second.last_heartbeat_ms = now_ms;
    }
}

std::vector<WatchdogAction> WatchdogSupervisor::check(uint32_t now_ms) {
    std::vector<WatchdogAction> actions;
    for (auto& [name, task] : tasks_) {
        (void)name;
        if (now_ms - task.last_heartbeat_ms > task.timeout_ms) {
            actions.push_back(WatchdogAction::ResetC3);
            task.last_heartbeat_ms = now_ms;
        }
    }
    return actions;
}

void WatchdogSupervisor::record_rv_alive(uint32_t now_ms) {
    last_rv_alive_ms_ = now_ms;
    rv_alive_miss_count_ = 0;
}

std::vector<WatchdogAction> WatchdogSupervisor::note_rv_alive_missed() {
    ++rv_alive_miss_count_;
    if (rv_alive_miss_count_ >= 3) {
        rv_alive_miss_count_ = 0;
        return {WatchdogAction::PowerCycleRv};
    }
    return {};
}

std::vector<WatchdogAction> WatchdogSupervisor::note_brownout() {
    return {WatchdogAction::ColdBoot};
}

}  // namespace hs::c3
