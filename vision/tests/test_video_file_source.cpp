// firmware/vision/tests/test_video_file_source.cpp
#include "vision/video_file_source.h"
#include <cassert>
#include <cstdio>

int main() {
    const char* path = std::getenv("HS_FIXTURE_DIR");
    assert(path && "HS_FIXTURE_DIR not set");
    std::string mp4 = std::string(path) + "/synth_blob_30f.mp4";

    hs::vision::VideoFileSource src(mp4);
    assert(src.open());
    assert(src.width() == 640);
    assert(src.height() == 480);
    assert(src.fps() > 29.0 && src.fps() < 31.0);

    int count = 0;
    hs::vision::Frame f;
    uint64_t prev_ts = 0;
    while (src.read(f)) {
        assert(f.valid());
        assert(f.bgr.cols == 640 && f.bgr.rows == 480);
        if (count > 0) assert(f.ts_us > prev_ts);
        prev_ts = f.ts_us;
        ++count;
    }
    assert(count == 60);
    src.close();
    std::printf("ok: %d frames\n", count);
    return 0;
}
