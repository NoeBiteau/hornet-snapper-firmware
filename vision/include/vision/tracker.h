// firmware/vision/include/vision/tracker.h
#pragma once
#include "vision/detector.h"
#include <opencv2/tracking.hpp>
#include <deque>
#include <memory>
#include <vector>

namespace hs::vision {

struct Track {
    uint8_t id;
    ClassId cls;
    cv::Rect bbox;
    float conf;
    int   age_frames = 0;
    int   frames_since_nn_match = 0;
    std::deque<cv::Rect> bbox_history;          // last 30
    std::deque<ClassId>  class_history;         // last 5
    std::deque<float>    conf_history;          // last 5
    cv::Ptr<cv::legacy::Tracker> mosse;
    bool alive = true;
};

class Tracker {
public:
    struct Params {
        int   max_tracks = 5;
        float iou_match_threshold = 0.3f;
        int   max_lost_frames = 5;
        int   max_age_frames  = 90;
    };
    explicit Tracker(Params p = {});
    void update(const cv::Mat& frame,
                const std::vector<Detection>& nn_dets,
                bool nn_ran_this_frame);
    const std::vector<std::shared_ptr<Track>>& tracks() const { return tracks_; }
private:
    Params p_;
    uint8_t next_id_ = 1;
    std::vector<std::shared_ptr<Track>> tracks_;
    static float iou(const cv::Rect& a, const cv::Rect& b);
};

}  // namespace hs::vision
