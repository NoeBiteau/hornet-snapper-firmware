// firmware/vision/src/tracker.cpp
#include "vision/tracker.h"
#include <opencv2/imgproc.hpp>
#include <algorithm>

namespace hs::vision {
namespace {
cv::Rect clamp_rect(const cv::Rect& r, const cv::Size& bounds) {
    return r & cv::Rect(0, 0, bounds.width, bounds.height);
}

cv::Mat crop_patch(const cv::Mat& frame, const cv::Rect& bbox) {
    cv::Rect clipped = clamp_rect(bbox, frame.size());
    if (clipped.empty()) return {};
    return frame(clipped).clone();
}

bool match_patch_near(const cv::Mat& frame, const cv::Mat& patch,
                      const cv::Rect& previous, cv::Rect& matched) {
    if (patch.empty() || previous.empty()) return false;

    const int pad = std::max(previous.width, previous.height) * 3;
    cv::Rect search(previous.x - pad, previous.y - pad,
                    previous.width + 2 * pad, previous.height + 2 * pad);
    search = clamp_rect(search, frame.size());
    if (search.width < patch.cols || search.height < patch.rows) return false;

    cv::Mat result;
    cv::matchTemplate(frame(search), patch, result, cv::TM_SQDIFF_NORMED);

    double min_val = 1.0;
    cv::Point min_loc;
    cv::minMaxLoc(result, &min_val, nullptr, &min_loc, nullptr);
    if (min_val > 0.25) return false;

    matched = cv::Rect(search.x + min_loc.x, search.y + min_loc.y,
                       patch.cols, patch.rows);
    return true;
}
}  // namespace

float Tracker::iou(const cv::Rect& a, const cv::Rect& b) {
    cv::Rect i = a & b;
    if (i.area() == 0) return 0.0f;
    return (float)i.area() / (float)(a.area() + b.area() - i.area());
}

Tracker::Tracker() : Tracker(Params{}) {}

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
            t->appearance_patch = crop_patch(frame, t->bbox);
        } else {
            cv::Rect matched;
            if (!nn_ran && match_patch_near(frame, t->appearance_patch, t->bbox, matched)) {
                t->bbox = matched;
                t->bbox_history.push_back(t->bbox);
                if (t->bbox_history.size() > 30) t->bbox_history.pop_front();
                t->appearance_patch = crop_patch(frame, t->bbox);
            } else {
                t->alive = false;
            }
        }
        ++t->age_frames;
    }

    if (nn_ran) {
        std::vector<bool> det_used(dets.size(), false);
        std::vector<bool> track_matched(tracks_.size(), false);
        for (size_t track_idx = 0; track_idx < tracks_.size(); ++track_idx) {
            auto& t = tracks_[track_idx];
            if (!t->alive) continue;
            int  best = -1; float best_iou = 0;
            for (size_t i = 0; i < dets.size(); ++i) {
                if (det_used[i]) continue;
                float u = iou(t->bbox, dets[i].bbox);
                if (u > best_iou) { best_iou = u; best = (int)i; }
            }
            if (best >= 0 && best_iou >= p_.iou_match_threshold) {
                det_used[best] = true;
                track_matched[track_idx] = true;
                t->frames_since_nn_match = 0;
                t->cls = dets[best].cls;
                t->conf = dets[best].confidence;
                t->class_history.push_back(t->cls);
                if (t->class_history.size() > 5) t->class_history.pop_front();
                t->conf_history.push_back(t->conf);
                if (t->conf_history.size() > 5) t->conf_history.pop_front();
                // Reseed MOSSE with the corrected box.
                t->mosse = cv::legacy::TrackerMOSSE::create();
                cv::Rect2d corrected_box(dets[best].bbox);
                t->mosse->init(frame, corrected_box);
                t->bbox = dets[best].bbox;
                t->appearance_patch = crop_patch(frame, t->bbox);
            }
        }
        for (size_t track_idx = 0; track_idx < tracks_.size(); ++track_idx) {
            if (tracks_[track_idx]->alive && !track_matched[track_idx]) {
                ++tracks_[track_idx]->frames_since_nn_match;
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
            cv::Rect2d initial_box(dets[i].bbox);
            t->mosse->init(frame, initial_box);
            t->appearance_patch = crop_patch(frame, t->bbox);
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
