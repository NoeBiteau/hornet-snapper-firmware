// firmware/vision/tests/test_pipeline_e2e.cpp
// End-to-end test: drives hornet_snapper_rv1106 as a subprocess with a mock://
// script and asserts that at least one fire event is recorded in events.jsonl.
//
// Mock script uses stationary bbox (x=100, y=200, w=40, h=40) so velocity stays
// ~0 and the confirmer fires. The synth video's moving white square triggers the
// motion gate; the NN call schedule (frame_idx % 6 == 0) selects which script
// slots are consumed.
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;

int main() {
    // --- locate binary and fixture ----------------------------------------
    const char* hs_bin_env = std::getenv("HS_BIN");
    assert(hs_bin_env && "HS_BIN not set");
    std::string hs_bin(hs_bin_env);

    const char* fixture_dir = std::getenv("HS_FIXTURE_DIR");
    assert(fixture_dir && "HS_FIXTURE_DIR not set");
    std::string video = std::string(fixture_dir) + "/synth_blob_30f.mp4";

    // --- temp working directory -------------------------------------------
    fs::path tmp = fs::temp_directory_path() / "hs_e2e_test";
    fs::remove_all(tmp);
    fs::create_directories(tmp);
    fs::create_directories(tmp / "clips");

    // --- generate mock script (40 entries, i=10..49) ----------------------
    // Stationary bbox so velocity ~0 => confirmer fires.
    // Entries where i%6==0: detection with cls=Velutina (2), conf=0.9, x=100.
    // All other entries: empty (no detection).
    std::ostringstream s;
    s << "[";
    bool first = true;
    for (int i = 10; i < 50; ++i) {
        if (i % 6 != 0) {
            s << (first ? "" : ",") << "[]";
            first = false;
            continue;
        }
        int x = 100; // stationary — velocity ~0 so confirmer fires
        s << (first ? "" : ",")
          << "[{\"x\":" << x << ",\"y\":200,\"w\":40,\"h\":40"
          << ",\"conf\":0.9,\"cls\":2}]";
        first = false;
    }
    s << "]";

    fs::path script_path = tmp / "mock_script.json";
    {
        std::ofstream sf(script_path);
        sf << s.str();
    }

    // --- run the binary ---------------------------------------------------
    std::string data_dir = tmp.string();
    std::string model_uri = "mock://" + script_path.string();
    std::string cmd = "\"" + hs_bin + "\""
                    + " --video \"" + video + "\""
                    + " --model \"" + model_uri + "\""
                    + " --data-dir \"" + data_dir + "\"";
    std::printf("running: %s\n", cmd.c_str());
    int rc = std::system(cmd.c_str());
    std::printf("binary exit code: %d\n", rc);
    assert(rc == 0 && "binary exited non-zero");

    // --- check events.jsonl -----------------------------------------------
    fs::path evlog = tmp / "events.jsonl";
    assert(fs::exists(evlog) && "events.jsonl not created");

    int events = 0;
    std::ifstream in(evlog);
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        auto j = nlohmann::json::parse(line, nullptr, /*exceptions=*/false);
        if (j.is_discarded()) continue;
        if (j.value("fire", false)) {
            ++events;
            std::printf("fire event: %s\n", line.c_str());
        }
    }
    std::printf("total fire events: %d\n", events);
    assert(events >= 1 && "expected at least one fire event in events.jsonl");

    // --- check that at least one clip mp4 exists --------------------------
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
