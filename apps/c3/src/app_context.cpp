#include "app_context.h"

extern "C" {
#include "hal/uart.h"
#include "hal/time.h"
#include "uart_frame.h"
}

#include <array>
#include <sstream>

namespace hs::c3 {

AppContext::Options AppContext::Options::memory_only() {
    Options opts;
    opts.memory_storage = true;
    return opts;
}

AppContext::AppContext() : AppContext(Options{}) {}

AppContext::AppContext(Options options)
    : options_(std::move(options)),
      storage_(options_.memory_storage
                   ? StorageRing::memory(options_.max_storage_events)
                   : StorageRing(options_.event_log_path, options_.max_storage_events)),
      uart_port_(hs_uart_open(options_.uart_port_id, options_.uart_baud)) {}

void AppContext::emit_event(const hs_event_t& event) {
    std::array<uint8_t, HS_EVENT_WIRE_BYTES> payload{};
    hs_event_encode(&event, payload.data());

    hs_uart_frame_t frame{
        HS_UART_MSG_EVENT,
        uart_seq_++,
        HS_EVENT_WIRE_BYTES,
        payload.data(),
    };
    std::array<uint8_t, HS_EVENT_WIRE_BYTES + HS_UART_OVERHEAD_BYTES> encoded{};
    int n = hs_uart_encode(&frame, encoded.data(), encoded.size());
    if (n <= 0) {
        log("uart encode failed");
        return;
    }

    uart_frames_.push_back(std::vector<uint8_t>(encoded.begin(), encoded.begin() + n));
    hs_uart_write(uart_port_, encoded.data(), static_cast<std::size_t>(n));

    storage_.append(event);
    event_bus_.publish(BusEvent::uart_event(event));

    std::ostringstream line;
    if (event.type == HS_EVT_TYPE_FIRE) {
        line << "fire event ts=" << event.timestamp << " class=" << static_cast<int>(event.class_id)
             << " conf=" << static_cast<int>(event.confidence) << " fired=" << static_cast<int>(event.fired);
    } else if (event.type == HS_EVT_TYPE_FAULT) {
        line << "fault event ts=" << event.timestamp << " fault=" << static_cast<int>(event.flags);
    } else {
        line << "event type=" << static_cast<int>(event.type) << " ts=" << event.timestamp;
    }
    log(line.str());
}

void AppContext::log(std::string line) {
    log_lines_.push_back(std::move(line));
}

uint32_t AppContext::now_unix() const {
    if (options_.fixed_unix_time != 0) {
        return options_.fixed_unix_time;
    }
    return hs_time_unix();
}

}  // namespace hs::c3
