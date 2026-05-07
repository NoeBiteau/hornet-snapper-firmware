// firmware/vision/src/onnx_detector.cpp
#include "vision/onnx_detector.h"

#include <opencv2/imgproc.hpp>
#include <opencv2/dnn.hpp>

#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <vector>

namespace hs::vision {

// ---------------------------------------------------------------------------
// Ultralytics YOLO11 class-index → hs::vision::ClassId mapping.
// The training config must match this order:
//   0 = bee, 1 = velutina, 2 = crabro, 3 = other_vespa
// Anything out of range is mapped to Background (0).
// ---------------------------------------------------------------------------
static ClassId yolo_class_to_id(int cls) {
    switch (cls) {
        case 0:  return ClassId::Bee;
        case 1:  return ClassId::Velutina;
        case 2:  return ClassId::Crabro;
        case 3:  return ClassId::OtherVespa;
        default: return ClassId::Background;
    }
}

// ---------------------------------------------------------------------------
OnnxDetector::OnnxDetector(std::string model_path,
                            float conf_threshold,
                            float iou_threshold)
    : model_path_(std::move(model_path))
    , conf_threshold_(conf_threshold)
    , iou_threshold_(iou_threshold)
    , env_(ORT_LOGGING_LEVEL_WARNING, "hs_vision")
{}

// ---------------------------------------------------------------------------
bool OnnxDetector::init() {
    try {
        Ort::SessionOptions opts;
        opts.SetIntraOpNumThreads(1);
        opts.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_EXTENDED);

        session_ = Ort::Session(env_, model_path_.c_str(), opts);

        Ort::AllocatorWithDefaultOptions alloc;

        // Cache input names
        const size_t num_inputs = session_.GetInputCount();
        input_names_str_.reserve(num_inputs);
        for (size_t i = 0; i < num_inputs; ++i) {
            auto name = session_.GetInputNameAllocated(i, alloc);
            input_names_str_.emplace_back(name.get());
        }

        // Cache output names
        const size_t num_outputs = session_.GetOutputCount();
        output_names_str_.reserve(num_outputs);
        for (size_t i = 0; i < num_outputs; ++i) {
            auto name = session_.GetOutputNameAllocated(i, alloc);
            output_names_str_.emplace_back(name.get());
        }

        return true;
    } catch (const Ort::Exception& e) {
        return false;
    }
}

// ---------------------------------------------------------------------------
std::vector<Detection> OnnxDetector::infer(const cv::Mat& bgr_frame,
                                            const std::vector<cv::Rect>& /*rois*/) {
    // 1. Pre-process: resize to 640×640, BGR→RGB, normalize to [0,1]
    cv::Mat resized;
    cv::resize(bgr_frame, resized, cv::Size(INPUT_W, INPUT_H));

    cv::Mat rgb;
    cv::cvtColor(resized, rgb, cv::COLOR_BGR2RGB);

    cv::Mat fp32;
    rgb.convertTo(fp32, CV_32F, 1.0 / 255.0);

    // HWC → CHW (NCHW layout expected by ONNX model)
    // Split into channels, then flatten in order R, G, B
    std::vector<cv::Mat> channels(3);
    cv::split(fp32, channels);

    std::vector<float> input_data(3 * INPUT_H * INPUT_W);
    for (int c = 0; c < 3; ++c) {
        std::memcpy(input_data.data() + c * INPUT_H * INPUT_W,
                    channels[c].ptr<float>(),
                    INPUT_H * INPUT_W * sizeof(float));
    }

    // 2. Build input tensor
    std::array<int64_t, 4> input_shape{1, 3, INPUT_H, INPUT_W};
    auto memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
        memory_info,
        input_data.data(), input_data.size(),
        input_shape.data(), input_shape.size());

    // 3. Run inference
    std::vector<const char*> input_names_cstr;
    input_names_cstr.reserve(input_names_str_.size());
    for (const auto& s : input_names_str_) input_names_cstr.push_back(s.c_str());

    std::vector<const char*> output_names_cstr;
    output_names_cstr.reserve(output_names_str_.size());
    for (const auto& s : output_names_str_) output_names_cstr.push_back(s.c_str());

    auto output_tensors = session_.Run(
        Ort::RunOptions{nullptr},
        input_names_cstr.data(),  &input_tensor, 1,
        output_names_cstr.data(), output_names_cstr.size());

    // 4. Decode output tensor  [1, K, 8400]
    //    K = 4 (xywh) + num_classes
    //    Layout is transposed: element [k, i] is at data[k * N + i]
    const float* data = output_tensors[0].GetTensorData<float>();
    auto shape = output_tensors[0].GetTensorTypeAndShapeInfo().GetShape();
    // shape = {1, K, N}
    const int K = static_cast<int>(shape[1]);   // rows = 4 + num_classes
    const int N = static_cast<int>(shape[2]);   // 8400 anchors
    const int num_classes = K - 4;

    // Original image dimensions (for scaling boxes back)
    const float scale_x = static_cast<float>(bgr_frame.cols) / INPUT_W;
    const float scale_y = static_cast<float>(bgr_frame.rows) / INPUT_H;

    std::vector<cv::Rect2f> boxes;
    std::vector<float>      scores;
    std::vector<int>        class_ids;

    boxes.reserve(256);
    scores.reserve(256);
    class_ids.reserve(256);

    for (int i = 0; i < N; ++i) {
        // xywh in model-input space
        float cx = data[0 * N + i];
        float cy = data[1 * N + i];
        float w  = data[2 * N + i];
        float h  = data[3 * N + i];

        // Find best class
        float best_score = 0.0f;
        int   best_cls   = 0;
        for (int c = 0; c < num_classes; ++c) {
            float s = data[(4 + c) * N + i];
            if (s > best_score) {
                best_score = s;
                best_cls   = c;
            }
        }

        if (best_score < conf_threshold_) continue;

        // Convert cx,cy,w,h → top-left x,y in original image space
        float x = (cx - w / 2.0f) * scale_x;
        float y = (cy - h / 2.0f) * scale_y;
        float bw = w * scale_x;
        float bh = h * scale_y;

        boxes.push_back({x, y, bw, bh});
        scores.push_back(best_score);
        class_ids.push_back(best_cls);
    }

    // 5. NMS
    std::vector<int> indices;
    cv::dnn::NMSBoxes(boxes, scores, conf_threshold_, iou_threshold_, indices);

    std::vector<Detection> detections;
    detections.reserve(indices.size());
    for (int idx : indices) {
        Detection d;
        d.bbox = cv::Rect(
            static_cast<int>(boxes[idx].x),
            static_cast<int>(boxes[idx].y),
            static_cast<int>(boxes[idx].width),
            static_cast<int>(boxes[idx].height));
        d.confidence = scores[idx];
        d.cls        = yolo_class_to_id(class_ids[idx]);
        detections.push_back(d);
    }

    return detections;
}

}  // namespace hs::vision
