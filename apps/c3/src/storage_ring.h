#ifndef HS_C3_STORAGE_RING_H
#define HS_C3_STORAGE_RING_H

extern "C" {
#include "event.h"
}

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <vector>

namespace hs::c3 {

class StorageRing {
public:
    StorageRing();
    explicit StorageRing(std::filesystem::path path, std::size_t max_records = 1000);

    static StorageRing memory(std::size_t max_records = 1000);
    static std::filesystem::path default_path();

    void append(const hs_event_t& event);
    std::vector<hs_event_t> load_all() const;
    void prune(uint32_t now);
    std::size_t size() const;

private:
    StorageRing(std::size_t max_records, bool memory_only);

    void write_all(const std::vector<hs_event_t>& events);
    std::vector<hs_event_t> trim_to_capacity(std::vector<hs_event_t> events) const;

    std::filesystem::path path_;
    std::size_t max_records_ = 1000;
    bool memory_only_ = false;
    std::vector<hs_event_t> memory_events_;
};

}  // namespace hs::c3

#endif
