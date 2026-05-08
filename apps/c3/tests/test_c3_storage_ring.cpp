#include "storage_ring.h"

extern "C" {
#include "event.h"
}

#include <cassert>
#include <filesystem>

int main() {
    using namespace hs::c3;

    auto path = std::filesystem::temp_directory_path() / "hornet-snapper-c3-test-events.log";
    std::filesystem::remove(path);
    StorageRing ring(path, 1000);

    for (uint32_t i = 0; i < 1010; ++i) {
        hs_event_t e{};
        e.timestamp = i;
        e.type = HS_EVT_TYPE_FIRE;
        e.fired = 1;
        ring.append(e);
    }
    assert(ring.size() == 1000);
    auto all = ring.load_all();
    assert(all.front().timestamp == 10);
    assert(all.back().timestamp == 1009);

    constexpr uint32_t day = 24u * 60u * 60u;
    constexpr uint32_t fresh_base = 40u * day;
    for (uint32_t i = 0; i < 20; ++i) {
        hs_event_t e{};
        e.timestamp = fresh_base + i;
        e.type = HS_EVT_TYPE_FAULT;
        ring.append(e);
    }
    ring.prune(fresh_base + day);
    all = ring.load_all();
    assert(!all.empty());
    assert(all.front().timestamp >= fresh_base);
    assert(all.back().timestamp == fresh_base + 19);

    std::filesystem::remove(path);
    return 0;
}
