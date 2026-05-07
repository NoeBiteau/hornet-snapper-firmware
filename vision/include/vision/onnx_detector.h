// firmware/vision/include/vision/onnx_detector.h
#pragma once
#include "vision/detector.h"

#include <onnxruntime_cxx_api.h>
#include <string>
#include <vector>

namespace hs::vision {

// IDetector implementation backed by ONNX Runtime (FP32).
// Expects a YOLO11 ultralytics export: input  [1,3,640,640]
//                                       output [1, K, 8400]  (transposed layout)
// where K = 4 (xywh) + num_classes.
class OnnxDetector : public IDetector {
public:
    explicit OnnxDetector(std::string model_path,
                          float conf_threshold = 0.25f,
                          float iou_threshold  = 0.45f);

    bool init() override;

    // Runs inference on bgr_frame (any size; resized to 640×640 internally).
    // rois is currently ignored (reserved for future ROI masking).
    std::vector<Detection> infer(const cv::Mat& bgr_frame,
                                 const std::vector<cv::Rect>& rois) override;

private:
    std::string model_path_;
    float       conf_threshold_;
    float       iou_threshold_;

    Ort::Env     env_;
    Ort::Session session_{nullptr};

    // Cached I/O name strings (owned by the session allocator; kept alive here)
    std::vector<std::string>      input_names_str_;
    std::vector<std::string>      output_names_str_;

    static constexpr int INPUT_W = 640;
    static constexpr int INPUT_H = 640;
};

}  // namespace hs::vision
