// firmware/vision/src/mock_detector.cpp
#include "vision/mock_detector.h"

namespace hs::vision {

MockDetector::MockDetector(std::vector<std::vector<Detection>> script)
    : script_(std::move(script)) {}

bool MockDetector::init() { return true; }

std::vector<Detection> MockDetector::infer(const cv::Mat& /*bgr_frame*/,
                                            const std::vector<cv::Rect>& /*rois*/) {
    if (index_ >= script_.size()) return {};
    return script_[index_++];
}

}  // namespace hs::vision
