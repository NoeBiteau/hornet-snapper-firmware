// firmware/vision/tests/test_event_log.cpp
#include "vision/event_log.h"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <nlohmann/json.hpp>
namespace fs = std::filesystem;

int main() {
    fs::path tmp = fs::temp_directory_path() / "hs_evlog_test.jsonl";
    fs::remove(tmp);
    hs::vision::EventLog log(tmp.string());
    assert(log.open());

    hs::vision::ConfirmDecision d;
    d.fire = true; d.track_id = 7; d.cls = hs::vision::ClassId::Velutina;
    d.confidence = 0.83f; d.bbox = cv::Rect(10, 20, 30, 40);
    log.write(d, 1234567890ULL, "/data/clips/2026-05-07/abc.mp4");
    log.close();

    std::ifstream in(tmp);
    std::string line; std::getline(in, line);
    auto j = nlohmann::json::parse(line);
    assert(j["fire"] == true);
    assert(j["track_id"] == 7);
    assert(j["class_id"] == 2);
    assert(j["bbox"] == nlohmann::json::array({10, 20, 30, 40}));
    return 0;
}
