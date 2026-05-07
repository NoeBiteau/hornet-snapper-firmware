// firmware/vision/include/vision/event_log.h
#pragma once
#include "vision/confirmer.h"
#include <fstream>
#include <string>

namespace hs::vision {

class EventLog {
public:
    explicit EventLog(std::string path);
    ~EventLog();
    bool open();
    void write(const ConfirmDecision& d, uint64_t ts_us, const std::string& clip_path);
    void close();
private:
    std::string path_;
    std::ofstream f_;
};

}  // namespace hs::vision
