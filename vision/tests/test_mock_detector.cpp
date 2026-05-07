// firmware/vision/tests/test_mock_detector.cpp
#include "vision/mock_detector.h"
#include <cassert>
#include <cstdio>

using namespace hs::vision;

int main() {
    // Build a 3-frame script
    std::vector<std::vector<Detection>> script = {
        { Detection{ cv::Rect(0, 0, 10, 10), 0.9f, ClassId::Bee } },
        { Detection{ cv::Rect(5, 5, 20, 20), 0.8f, ClassId::Velutina },
          Detection{ cv::Rect(1, 1,  5,  5), 0.7f, ClassId::Crabro } },
        { Detection{ cv::Rect(0, 0,  1,  1), 0.6f, ClassId::OtherVespa } },
    };

    MockDetector det(script);
    assert(det.init());

    cv::Mat dummy;
    std::vector<cv::Rect> rois;

    // Invocation 1
    auto r0 = det.infer(dummy, rois);
    assert(r0.size() == 1);
    assert(r0[0].cls == ClassId::Bee);
    assert(r0[0].confidence == 0.9f);

    // Invocation 2
    auto r1 = det.infer(dummy, rois);
    assert(r1.size() == 2);
    assert(r1[0].cls == ClassId::Velutina);
    assert(r1[1].cls == ClassId::Crabro);

    // Invocation 3
    auto r2 = det.infer(dummy, rois);
    assert(r2.size() == 1);
    assert(r2[0].cls == ClassId::OtherVespa);

    // Post-script: must return empty
    auto r3 = det.infer(dummy, rois);
    assert(r3.empty());

    std::puts("test_mock_detector PASSED");
    return 0;
}
