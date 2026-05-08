#include "storage_ring.h"

#include <algorithm>
#include <array>
#include <fstream>

namespace hs::c3 {

namespace {
constexpr uint32_t kRetentionSeconds = 30u * 24u * 60u * 60u;

uint16_t read_le16(const uint8_t* p) {
    return static_cast<uint16_t>(p[0] | (static_cast<uint16_t>(p[1]) << 8));
}

hs_event_t decode_event(const uint8_t* in) {
    hs_event_t e{};
    e.timestamp = static_cast<uint32_t>(in[0]) |
                  (static_cast<uint32_t>(in[1]) << 8) |
                  (static_cast<uint32_t>(in[2]) << 16) |
                  (static_cast<uint32_t>(in[3]) << 24);
    e.type = in[4];
    e.class_id = in[5];
    e.confidence = in[6];
    e.fired = in[7];
    for (int i = 0; i < 4; ++i) {
        e.bbox[i] = read_le16(in + 8 + 2 * i);
    }
    e.flags = in[16];
    e.reserved = in[17];
    e.battery_mv = read_le16(in + 18);
    e.reserved2 = read_le16(in + 20);
    return e;
}
}  // namespace

StorageRing::StorageRing() : StorageRing(default_path(), 1000) {}

StorageRing::StorageRing(std::filesystem::path path, std::size_t max_records)
    : path_(std::move(path)), max_records_(max_records) {}

StorageRing::StorageRing(std::size_t max_records, bool memory_only)
    : max_records_(max_records), memory_only_(memory_only) {}

StorageRing StorageRing::memory(std::size_t max_records) {
    return StorageRing(max_records, true);
}

std::filesystem::path StorageRing::default_path() {
    return "/tmp/hornet-snapper-c3/events.log";
}

void StorageRing::append(const hs_event_t& event) {
    auto events = load_all();
    events.push_back(event);
    write_all(trim_to_capacity(std::move(events)));
}

std::vector<hs_event_t> StorageRing::load_all() const {
    if (memory_only_) {
        return memory_events_;
    }
    std::vector<hs_event_t> events;
    std::ifstream in(path_, std::ios::binary);
    if (!in) {
        return events;
    }
    std::array<uint8_t, HS_EVENT_WIRE_BYTES> raw{};
    while (in.read(reinterpret_cast<char*>(raw.data()), raw.size())) {
        events.push_back(decode_event(raw.data()));
    }
    return events;
}

void StorageRing::prune(uint32_t now) {
    auto events = load_all();
    events.erase(std::remove_if(events.begin(), events.end(), [now](const hs_event_t& e) {
                     return e.timestamp + kRetentionSeconds < now;
                 }),
                 events.end());
    write_all(trim_to_capacity(std::move(events)));
}

std::size_t StorageRing::size() const {
    return load_all().size();
}

void StorageRing::write_all(const std::vector<hs_event_t>& events) {
    if (memory_only_) {
        memory_events_ = events;
        return;
    }
    if (!path_.parent_path().empty()) {
        std::filesystem::create_directories(path_.parent_path());
    }
    std::ofstream out(path_, std::ios::binary | std::ios::trunc);
    for (const auto& event : events) {
        std::array<uint8_t, HS_EVENT_WIRE_BYTES> raw{};
        hs_event_encode(&event, raw.data());
        out.write(reinterpret_cast<const char*>(raw.data()), raw.size());
    }
}

std::vector<hs_event_t> StorageRing::trim_to_capacity(std::vector<hs_event_t> events) const {
    if (max_records_ == 0) {
        events.clear();
        return events;
    }
    if (events.size() > max_records_) {
        events.erase(events.begin(), events.end() - static_cast<std::ptrdiff_t>(max_records_));
    }
    return events;
}

}  // namespace hs::c3
