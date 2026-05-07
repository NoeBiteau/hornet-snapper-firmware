// firmware/vision/src/tracker.cpp
#include "vision/tracker.h"
#include <opencv2/tracking/tracking_legacy.hpp>
#include <algorithm>

namespace hs::vision {

float Tracker::iou(const cv::Rect& a, const cv::Rect& b) {
    cv::Rect i = a & b;
    if (i.area() == 0) return 0.0f;
    return (float)i.area() / (float)(a.area() + b.area() - i.area());
}

Tracker::Tracker(Params p) : p_(p) {}

void Tracker::update(const cv::Mat& frame,
                     const std::vector<Detection>& dets,
                     bool nn_ran) {
    // Advance every track via MOSSE.
    for (auto& t : tracks_) {
        cv::Rect2d r = t->bbox;
        bool ok = t->mosse && t->mosse->update(frame, r);
        if (ok) {
            t->bbox = cv::Rect((int)r.x, (int)r.y, (int)r.width, (int)r.height);
            t->bbox_history.push_back(t->bbox);
            if (t->bbox_history.size() > 30) t->bbox_history.pop_front();
        } else {
            t->alive = false;
        }
        ++t->age_frames;
        if (!nn_ran) ++t->frames_since_nn_match;
    }

    if (nn_ran) {
        std::vector<bool> det_used(dets.size(), false);
        for (auto& t : tracks_) {
            if (!t->alive) continue;
            int  best = -1; float best_iou = 0;
            for (size_t i = 0; i < dets.size(); ++i) {
                if (det_used[i]) continue;
                float u = iou(t->bbox, dets[i].bbox);
                if (u > best_iou) { best_iou = u; best = (int)i; }
            }
            if (best >= 0 && best_iou >= p_.iou_match_threshold) {
                det_used[best] = true;
                t->frames_since_nn_match = 0;
                t->cls = dets[best].cls;
                t->conf = dets[best].confidence;
                t->class_history.push_back(t->cls);
                if (t->class_history.size() > 5) t->class_history.pop_front();
                t->conf_history.push_back(t->conf);
                if (t->conf_history.size() > 5) t->conf_history.pop_front();
                // Reseed MOSSE with the corrected box.
                t->mosse = cv::legacy::TrackerMOSSE::create();
                t->mosse->init(frame, dets[best].bbox);
                t->bbox = dets[best].bbox;
            }
        }
        for (size_t i = 0; i < dets.size(); ++i) {
            if (det_used[i]) continue;
            if (tracks_.size() >= (size_t)p_.max_tracks) break;
            auto t = std::make_shared<Track>();
            t->id = next_id_++;
            t->cls = dets[i].cls;
            t->conf = dets[i].confidence;
            t->bbox = dets[i].bbox;
            t->bbox_history.push_back(t->bbox);
            t->class_history.push_back(t->cls);
            t->conf_history.push_back(t->conf);
            t->mosse = cv::legacy::TrackerMOSSE::create();
            t->mosse->init(frame, dets[i].bbox);
            tracks_.push_back(t);
        }
    }

    // Drop dead tracks.
    tracks_.erase(std::remove_if(tracks_.begin(), tracks_.end(),
        [&](const std::shared_ptr<Track>& t) {
            if (!t->alive) return true;
            if (t->frames_since_nn_match > p_.max_lost_frames) return true;
            if (t->age_frames > p_.max_age_frames) return true;
            return false;
        }), tracks_.end());
}

}  // namespace hs::vision
