// firmware/vision/tests/test_tracker.cpp
#include "vision/tracker.h"
#include <cassert>
#include <opencv2/imgproc.hpp>
#include <cstdio>

int main() {
    using namespace hs::vision;
    Tracker tr;
    cv::Mat frame(480, 640, CV_8UC3, cv::Scalar(50, 60, 70));
    cv::Rect box(100, 200, 40, 40);

    // First frame: NN gives one velutina detection at box.
    cv::rectangle(frame, box, cv::Scalar(255, 255, 255), cv::FILLED);
    Detection d{box, 0.9f, ClassId::Velutina};
    tr.update(frame, {d}, true);
    assert(tr.tracks().size() == 1);

    // Move the box 8 px/frame for 6 frames; MOSSE only (no NN).
    for (int i = 1; i <= 6; ++i) {
        frame.setTo(cv::Scalar(50, 60, 70));
        cv::Rect b(100 + i * 8, 200, 40, 40);
        cv::rectangle(frame, b, cv::Scalar(255, 255, 255), cv::FILLED);
        tr.update(frame, {}, false);
    }
    assert(tr.tracks().size() == 1);
    auto& t = tr.tracks().front();
    // After 6 frames of 8 px/frame, expect bbox.x near 148 (100 + 48).
    std::printf("bbox.x after 6 frames: %d\n", t->bbox.x);
    assert(std::abs(t->bbox.x - 148) < 25);

    // 7th non-NN frame is fine (less than max_lost_frames).
    // After 6 frames without NN matches, frames_since_nn_match = 6 > 5 -> drop.
    frame.setTo(cv::Scalar(50, 60, 70));
    tr.update(frame, {}, true);    // NN ran, no detections -> still no match
    assert(tr.tracks().empty());
    return 0;
}
