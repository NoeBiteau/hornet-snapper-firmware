// firmware/vision/tests/test_public_headers_compile.cpp
#include "vision/clip_writer.h"
#include "vision/confirmer.h"
#include "vision/detector.h"
#include "vision/event_log.h"
#include "vision/frame.h"
#include "vision/frame_source.h"
#include "vision/libcamera_source.h"
#include "vision/mock_detector.h"
#include "vision/motion_gate.h"
#include "vision/onnx_detector.h"
#include "vision/tracker.h"
#include "vision/video_file_source.h"

#include <vector>

int main() {
    hs::vision::MotionGate gate(640, 480);
    hs::vision::Tracker tracker;
    hs::vision::Confirmer confirmer;
    hs::vision::MockDetector mock(std::vector<std::vector<hs::vision::Detection>>{});
    hs::vision::ClipWriter clip_writer(hs::vision::ClipWriter::Params{});
    hs::vision::EventLog event_log("events.jsonl");
    hs::vision::OnnxDetector onnx_detector("model.onnx");

    (void)gate;
    (void)tracker;
    (void)confirmer;
    (void)mock;
    (void)clip_writer;
    (void)event_log;
    (void)onnx_detector;
    return 0;
}
