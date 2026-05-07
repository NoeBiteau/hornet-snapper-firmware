// firmware/vision/include/vision/detector.h
#pragma once
#include <opencv2/core.hpp>
#include <cstdint>
#include <vector>

namespace hs::vision {

enum class ClassId : uint8_t {
    Background  = 0,
    Bee         = 1,
    Velutina    = 2,
    Crabro      = 3,
    OtherVespa  = 4,
};

struct Detection {
    cv::Rect bbox;
    float    confidence;
    ClassId  cls;
};

class IDetector {
public:
    virtual ~IDetector() = default;
    virtual bool init() = 0;
    virtual std::vector<Detection> infer(const cv::Mat& bgr_frame,
                                         const std::vector<cv::Rect>& rois) = 0;
};

}  // namespace hs::vision
