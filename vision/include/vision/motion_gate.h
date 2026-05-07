// firmware/vision/include/vision/motion_gate.h
#pragma once
#include "vision/frame.h"
#include <opencv2/core.hpp>
#include <vector>

namespace hs::vision {

struct MotionRoi {
    cv::Rect bbox;       // in source-frame coords
    int x() const { return bbox.x; }
    int y() const { return bbox.y; }
    int width()  const { return bbox.width; }
    int height() const { return bbox.height; }
    float area() const { return (float)bbox.area(); }
};

class MotionGate {
public:
    struct Params {
        float alpha = 0.05f;          // BG running-mean rate
        int   threshold = 25;         // diff threshold (8U)
        int   min_area = 50;          // px (in 160×120 detection space)
        int   max_area = 2000;
        float min_aspect = 0.4f;
        float max_aspect = 2.5f;
        int   detect_w = 160;         // downscale target
        int   detect_h = 120;
    };
    MotionGate(int src_w, int src_h);
    MotionGate(int src_w, int src_h, Params p);
    std::vector<MotionRoi> process(const Frame& f);
    void reset();
private:
    int src_w_, src_h_;
    Params p_;
    cv::Mat bg_;          // CV_32FC1, gray, in detect-space
    cv::Mat scratch_gray_, scratch_diff_, scratch_mask_;
    bool bg_ready_ = false;
};

}  // namespace hs::vision
