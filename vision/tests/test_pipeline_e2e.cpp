// firmware/vision/tests/test_pipeline_e2e.cpp
// End-to-end test: drives hornet_snapper_rv1106 as a subprocess with a mock://
// script and asserts that at least one fire event is recorded in events.jsonl.

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;

int main() {
    const char* hs_bin_env = std::getenv("HS_BIN");
    assert(hs_bin_env && "HS_BIN not set");
    std::string hs_bin(hs_bin_env);

    const char* fixture_dir = std::getenv("HS_FIXTURE_DIR");
    assert(fixture_dir && "HS_FIXTURE_DIR not set");
    std::string video = std::string(fixture_dir) + "/synth_blob_30f.mp4";

    fs::path tmp = fs::temp_directory_path() / "hs_e2e_test";
    fs::remove_all(tmp);
    fs::create_directories(tmp);
    fs::create_directories(tmp / "clips");

    // MockDetector consumes one script entry per infer() call, not per frame.
    // Each call gets candidates across the synthetic blob path so the test
    // does not depend on the exact frames selected by motion gating.
    const int xs[] = {66, 114, 162, 210, 258, 306, 354};
    std::ostringstream s;
    s << "[";
    for (int i = 0; i < 7; ++i) {
        s << (i == 0 ? "" : ",") << "[";
        for (int j = 0; j < 7; ++j) {
            s << (j == 0 ? "" : ",")
              << "{\"x\":" << xs[j] << ",\"y\":200,\"w\":40,\"h\":40"
              << ",\"conf\":0.9,\"cls\":2}";
        }
        s << "]";
    }
    s << "]";

    fs::path script_path = tmp / "mock_script.json";
    {
        std::ofstream sf(script_path);
        sf << s.str();
    }

    std::string data_dir = tmp.string();
    std::string model_uri = "mock://" + script_path.string();
    std::string cmd = "\"" + hs_bin + "\""
                    + " --video \"" + video + "\""
                    + " --model \"" + model_uri + "\""
                    + " --data-dir \"" + data_dir + "\""
                    + " --confirm-min-age-frames 1"
                    + " --confirm-class-min-match 1"
                    + " --cooldown-us 0"
                    + " --max-velocity-pps 1000";
    std::printf("running: %s\n", cmd.c_str());
    int rc = std::system(cmd.c_str());
    std::printf("binary exit code: %d\n", rc);
    assert(rc == 0 && "binary exited non-zero");

    fs::path evlog = tmp / "events.jsonl";
    assert(fs::exists(evlog) && "events.jsonl not created");

    int events = 0;
    std::ifstream in(evlog);
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        auto j = nlohmann::json::parse(line, nullptr, false);
        if (j.is_discarded()) continue;
        if (j.value("fire", false)) {
            ++events;
            std::printf("fire event: %s\n", line.c_str());
        }
    }
    std::printf("total fire events: %d\n", events);
    assert(events >= 1 && "expected at least one fire event in events.jsonl");

    int clips = 0;
    for (auto& entry : fs::recursive_directory_iterator(tmp / "clips")) {
        if (entry.path().extension() == ".mp4") {
            ++clips;
            std::printf("clip: %s\n", entry.path().c_str());
        }
    }
    assert(clips >= 1 && "expected at least one clip .mp4");

    std::puts("test_pipeline_e2e PASSED");
    return 0;
}
