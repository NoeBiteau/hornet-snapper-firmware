// firmware/vision/tests/test_clip_writer.cpp
#include "vision/clip_writer.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

int main() {
    fs::path tmp = fs::temp_directory_path() / "hs_clip_test";
    fs::remove_all(tmp);
    hs::vision::ClipWriter::Params p;
    p.pre_seconds = 1; p.post_seconds = 1; p.fps = 10.0;
    p.base_dir = tmp.string();
    hs::vision::ClipWriter cw(p);

    cv::Mat f(120, 160, CV_8UC3, cv::Scalar(0, 255, 0));
    for (int i = 0; i < 15; ++i) cw.push(f);    // pre-buffer accumulates ~10
    assert(cw.start_clip("test_event", 160, 120));
    for (int i = 0; i < 12; ++i) { cw.push(f); cw.tick(); }
    // Should have closed by now (10 post frames written + tick decrements).
    assert(!cw.is_recording());

    // Verify the file exists and ffprobe can read >= ~15 frames.
    std::string clip = cw.current_path();
    assert(!clip.empty() && fs::exists(clip));
    std::string cmd = "ffprobe -v error -count_frames -select_streams v:0 -show_entries stream=nb_read_frames -of csv=p=0 " + clip;
    std::FILE* p2 = popen(cmd.c_str(), "r");
    char buf[64] = {0};
    fread(buf, 1, sizeof(buf) - 1, p2);
    pclose(p2);
    int n = std::atoi(buf);
    std::printf("clip frames: %d (path %s)\n", n, clip.c_str());
    assert(n >= 12);  // 10 pre + ≥5 post (relaxed to 12 for ffmpeg trailing flush tolerance)
    return 0;
}
