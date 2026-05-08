#include "event_bus.h"

namespace hs::c3 {

BusEvent BusEvent::uart_event(const hs_event_t& event) {
    BusEvent out;
    out.kind = EventKind::UartEvent;
    out.event = event;
    return out;
}

BusEvent BusEvent::fault(FaultId id) {
    BusEvent out;
    out.kind = EventKind::Fault;
    out.fault_id = id;
    return out;
}

BusEvent BusEvent::command(uint32_t command_id) {
    BusEvent out;
    out.kind = EventKind::Command;
    out.command_id = command_id;
    return out;
}

void EventBus::publish(const BusEvent& event) {
    if (capacity_ == 0) {
        ++overflow_count_;
        return;
    }
    if (events_.size() == capacity_) {
        events_.pop_front();
        ++overflow_count_;
    }
    events_.push_back(event);
}

std::vector<BusEvent> EventBus::drain() {
    std::vector<BusEvent> out(events_.begin(), events_.end());
    events_.clear();
    return out;
}

}  // namespace hs::c3
