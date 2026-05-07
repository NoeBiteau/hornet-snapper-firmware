// firmware/vision/src/event_log.cpp
#include "vision/event_log.h"
#include <nlohmann/json.hpp>
#include <filesystem>

namespace hs::vision {

EventLog::EventLog(std::string path) : path_(std::move(path)) {}
EventLog::~EventLog() { close(); }

bool EventLog::open() {
    auto parent = std::filesystem::path(path_).parent_path();
    if (!parent.empty()) std::filesystem::create_directories(parent);
    f_.open(path_, std::ios::app);
    return f_.is_open();
}

void EventLog::close() { if (f_.is_open()) f_.close(); }

void EventLog::write(const ConfirmDecision& d, uint64_t ts_us, const std::string& clip_path) {
    if (!f_.is_open()) return;
    nlohmann::json j;
    j["ts_us"]    = ts_us;
    j["fire"]     = d.fire;
    j["track_id"] = d.track_id;
    j["class_id"] = (int)d.cls;
    j["conf"]     = d.confidence;
    j["bbox"]     = { d.bbox.x, d.bbox.y, d.bbox.width, d.bbox.height };
    j["clip"]     = clip_path;
    f_ << j.dump() << "\n";
    f_.flush();
}

}  // namespace hs::vision
