#ifndef HS_C3_APP_CONTEXT_H
#define HS_C3_APP_CONTEXT_H

#include "event_bus.h"
#include "fault_tree.h"
#include "power_gov.h"
#include "storage_ring.h"
#include "watchdog.h"

extern "C" {
#include "event.h"
}

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace hs::c3 {

class AppContext {
public:
    struct Options {
        bool memory_storage = false;
        std::filesystem::path event_log_path = StorageRing::default_path();
        std::size_t max_storage_events = 1000;
        uint16_t battery_mv = 12000;
        int uart_port_id = 0;
        uint32_t uart_baud = 115200;
        uint32_t fixed_unix_time = 0;

        static Options memory_only();
    };

    AppContext();
    explicit AppContext(Options options);

    void emit_event(const hs_event_t& event);
    void log(std::string line);

    EventBus& event_bus() { return event_bus_; }
    const EventBus& event_bus() const { return event_bus_; }

    StorageRing& storage() { return storage_; }
    const StorageRing& storage() const { return storage_; }

    PowerGovernor& power() { return power_; }
    const PowerGovernor& power() const { return power_; }

    WatchdogSupervisor& watchdog() { return watchdog_; }
    const WatchdogSupervisor& watchdog() const { return watchdog_; }

    hs_fault_inject_t& fault_injector() { return fault_injector_; }
    const hs_fault_inject_t& fault_injector() const { return fault_injector_; }

    const std::vector<std::vector<uint8_t>>& uart_frames() const { return uart_frames_; }
    const std::vector<std::string>& log_lines() const { return log_lines_; }

    uint16_t battery_mv() const { return options_.battery_mv; }
    uint32_t now_unix() const;

private:
    Options options_;
    EventBus event_bus_;
    StorageRing storage_;
    PowerGovernor power_;
    WatchdogSupervisor watchdog_;
    hs_fault_inject_t fault_injector_{};
    std::vector<std::vector<uint8_t>> uart_frames_;
    std::vector<std::string> log_lines_;
    int uart_port_ = 0;
    uint8_t uart_seq_ = 0;
};

}  // namespace hs::c3

#endif
