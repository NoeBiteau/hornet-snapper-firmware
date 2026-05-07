// firmware/vision/include/vision/frame.h
#pragma once
#include <opencv2/core.hpp>
#include <cstdint>

namespace hs::vision {

struct Frame {
    cv::Mat bgr;          // HxWx3 8U
    uint64_t ts_us = 0;   // monotonic capture timestamp, microseconds
    uint32_t seq = 0;     // monotonically increasing per source
    bool valid() const { return !bgr.empty(); }
};

}  // namespace hs::vision
