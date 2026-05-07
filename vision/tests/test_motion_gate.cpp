// firmware/vision/tests/test_motion_gate.cpp
#include "vision/motion_gate.h"
#include "vision/video_file_source.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>

int main() {
    const char* dir = std::getenv("HS_FIXTURE_DIR");
    assert(dir);
    hs::vision::VideoFileSource src(std::string(dir) + "/synth_blob_30f.mp4");
    assert(src.open());

    hs::vision::MotionGate gate(640, 480);
    int frames_with_blob = 0, frames_without = 0;
    hs::vision::Frame f;
    while (src.read(f)) {
        auto rois = gate.process(f);
        if (f.seq < 10 || f.seq >= 50) {
            // background only — but first frames seed the BG, so allow some FPs in 0..2
            if (f.seq >= 3 && f.seq < 10) frames_without += rois.empty() ? 0 : 1;
            if (f.seq >= 50)              frames_without += rois.empty() ? 0 : 1;
        } else if (f.seq >= 15) {  // blob present, BG settled
            if (!rois.empty()) {
                ++frames_with_blob;
                // blob is at x = 50 + (seq-10)*8, y=200, 40x40
                int expect_x = 50 + (int(f.seq) - 10) * 8;
                auto& r = rois.front();
                assert(r.x() < expect_x + 60 && r.x() + r.width() > expect_x - 20);
                assert(r.y() < 260 && r.y() + r.height() > 180);
            }
        }
    }
    std::printf("blob frames: %d, fp frames: %d\n", frames_with_blob, frames_without);
    std::fflush(stdout);
    assert(frames_with_blob >= 25);   // out of ~35 active blob frames
    assert(frames_without <= 3);      // small tolerance for BG settle artefacts
    return 0;
}
