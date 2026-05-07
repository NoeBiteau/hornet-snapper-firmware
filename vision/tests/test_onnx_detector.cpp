// firmware/vision/tests/test_onnx_detector.cpp
// Exit 77 when HS_MODEL_PATH is unset or the file does not exist
// (ctest SKIP_RETURN_CODE 77 → "Skipped")

#include "vision/onnx_detector.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <fstream>

using namespace hs::vision;

int main() {
    const char* model_path = std::getenv("HS_MODEL_PATH");
    if (!model_path || model_path[0] == '\0') {
        std::puts("SKIP: HS_MODEL_PATH not set");
        return 77;
    }
    {
        std::ifstream f(model_path);
        if (!f.good()) {
            std::printf("SKIP: model file not found: %s\n", model_path);
            return 77;
        }
    }

    OnnxDetector det(model_path, /*conf_threshold=*/0.25f, /*iou_threshold=*/0.45f);
    assert(det.init() && "OnnxDetector::init() failed");

    // Smoke-test: run on a blank 640×640 BGR frame, expect no crash
    cv::Mat blank(640, 640, CV_8UC3, cv::Scalar(114, 114, 114));
    std::vector<cv::Rect> rois;  // empty → whole frame
    auto results = det.infer(blank, rois);

    // We can't assert specific detections on a blank frame, but we do assert
    // the return type is well-formed.
    for (const auto& d : results) {
        assert(d.confidence >= 0.25f);
        assert(d.confidence <= 1.0f);
        assert(d.bbox.width > 0);
        assert(d.bbox.height > 0);
    }

    std::printf("test_onnx_detector PASSED  (detections=%zu)\n", results.size());
    return 0;
}
