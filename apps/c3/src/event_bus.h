#ifndef HS_C3_EVENT_BUS_H
#define HS_C3_EVENT_BUS_H

#include "fault_tree.h"

extern "C" {
#include "event.h"
}

#include <cstddef>
#include <cstdint>
#include <deque>
#include <vector>

namespace hs::c3 {

enum class EventKind {
    FireIrq,
    UartEvent,
    Fault,
    Command,
    WatchdogTick,
};

struct BusEvent {
    EventKind kind = EventKind::WatchdogTick;
    FaultId fault_id = FaultId::F1_MotorStall;
    hs_event_t event{};
    uint32_t command_id = 0;

    static BusEvent uart_event(const hs_event_t& event);
    static BusEvent fault(FaultId id);
    static BusEvent command(uint32_t command_id);
};

class EventBus {
public:
    explicit EventBus(std::size_t capacity = 64) : capacity_(capacity) {}

    void publish(const BusEvent& event);
    std::vector<BusEvent> drain();
    std::size_t size() const { return events_.size(); }
    std::size_t overflow_count() const { return overflow_count_; }

private:
    std::size_t capacity_;
    std::size_t overflow_count_ = 0;
    std::deque<BusEvent> events_;
};

}  // namespace hs::c3

#endif
