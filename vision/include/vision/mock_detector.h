// firmware/vision/include/vision/mock_detector.h
#pragma once
#include "vision/detector.h"
#include <vector>

namespace hs::vision {

// Replays a canned script of Detection lists, one list per infer() call.
// Once the script is exhausted, subsequent calls return an empty vector.
class MockDetector : public IDetector {
public:
    explicit MockDetector(std::vector<std::vector<Detection>> script);

    bool init() override;
    std::vector<Detection> infer(const cv::Mat& bgr_frame,
                                  const std::vector<cv::Rect>& rois) override;

private:
    std::vector<std::vector<Detection>> script_;
    std::size_t index_ = 0;
};

}  // namespace hs::vision
