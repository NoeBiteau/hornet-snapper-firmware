// firmware/vision/src/motion_gate.cpp
#include "vision/motion_gate.h"
#include <opencv2/imgproc.hpp>

namespace hs::vision {

MotionGate::MotionGate(int w, int h, Params p) : src_w_(w), src_h_(h), p_(p) {}

void MotionGate::reset() { bg_.release(); bg_ready_ = false; }

std::vector<MotionRoi> MotionGate::process(const Frame& f) {
    std::vector<MotionRoi> out;
    if (f.bgr.empty()) return out;

    cv::Mat gray;
    cv::cvtColor(f.bgr, gray, cv::COLOR_BGR2GRAY);
    cv::resize(gray, scratch_gray_, cv::Size(p_.detect_w, p_.detect_h), 0, 0, cv::INTER_AREA);

    cv::Mat gray_f;
    scratch_gray_.convertTo(gray_f, CV_32FC1);

    if (!bg_ready_) {
        bg_ = gray_f.clone();
        bg_ready_ = true;
        return out;   // cannot detect on the first frame
    }

    cv::absdiff(gray_f, bg_, scratch_diff_);
    cv::threshold(scratch_diff_, scratch_mask_, (double)p_.threshold, 255.0, cv::THRESH_BINARY);
    scratch_mask_.convertTo(scratch_mask_, CV_8UC1);
    cv::morphologyEx(scratch_mask_, scratch_mask_, cv::MORPH_OPEN,
                     cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3)));

    // Update BG (running mean)
    cv::addWeighted(bg_, 1.0f - p_.alpha, gray_f, p_.alpha, 0.0, bg_);

    // Connected components
    cv::Mat labels, stats, centroids;
    int n = cv::connectedComponentsWithStats(scratch_mask_, labels, stats, centroids, 8);

    const float sx = (float)src_w_ / (float)p_.detect_w;
    const float sy = (float)src_h_ / (float)p_.detect_h;

    for (int i = 1; i < n; ++i) {
        int area = stats.at<int>(i, cv::CC_STAT_AREA);
        if (area < p_.min_area || area > p_.max_area) continue;
        int x = stats.at<int>(i, cv::CC_STAT_LEFT);
        int y = stats.at<int>(i, cv::CC_STAT_TOP);
        int w = stats.at<int>(i, cv::CC_STAT_WIDTH);
        int h = stats.at<int>(i, cv::CC_STAT_HEIGHT);
        float aspect = (float)w / (float)h;
        if (aspect < p_.min_aspect || aspect > p_.max_aspect) continue;

        MotionRoi r;
        r.bbox = cv::Rect((int)(x * sx), (int)(y * sy),
                          (int)(w * sx), (int)(h * sy));
        out.push_back(r);
    }
    return out;
}

}  // namespace hs::vision
