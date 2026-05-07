// firmware/vision/include/vision/confirmer.h
#pragma once
#include "vision/tracker.h"
#include <vector>
#include <chrono>

namespace hs::vision {

struct ConfirmDecision {
    bool fire = false;
    uint8_t track_id = 0;
    ClassId cls = ClassId::Background;
    float confidence = 0.0f;
    cv::Rect bbox;
};

class Confirmer {
public:
    struct Params {
        int   class_n_of = 5;          // last N classifications
        int   class_min_match = 3;     // need >= this many
        ClassId target = ClassId::Velutina;
        float min_conf = 0.6f;
        int   min_age_frames = 9;      // 300 ms @ 30 fps
        float max_velocity_pps = 50.0f; // px/s in source coords
        std::vector<cv::Point> strike_zone; // empty = "all"
        uint64_t cooldown_us = 1'000'000;   // 1 s
    };
    explicit Confirmer(Params p = {});
    ConfirmDecision evaluate(const std::vector<std::shared_ptr<Track>>& tracks,
                             double fps,
                             uint64_t now_us);
private:
    Params p_;
    uint64_t last_fire_us_ = 0;
    static bool inside(const std::vector<cv::Point>& poly, cv::Point pt);
};

}  // namespace hs::vision
