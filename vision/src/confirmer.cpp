// firmware/vision/src/confirmer.cpp
#include "vision/confirmer.h"
#include <opencv2/imgproc.hpp>
#include <algorithm>

namespace hs::vision {

bool Confirmer::inside(const std::vector<cv::Point>& poly, cv::Point pt) {
    if (poly.empty()) return true;
    return cv::pointPolygonTest(poly, cv::Point2f(pt.x, pt.y), false) >= 0;
}

Confirmer::Confirmer() : Confirmer(Params{}) {}

Confirmer::Confirmer(Params p) : p_(std::move(p)) {}

ConfirmDecision Confirmer::evaluate(
        const std::vector<std::shared_ptr<Track>>& tracks,
        double fps,
        uint64_t now_us)
{
    ConfirmDecision dec;
    if (now_us - last_fire_us_ < p_.cooldown_us) return dec;

    for (auto& t : tracks) {
        if (t->age_frames < p_.min_age_frames) continue;

        int matches = (int)std::count(t->class_history.begin(),
                                      t->class_history.end(), p_.target);
        if (matches < p_.class_min_match) continue;

        // Mean recent confidence on target class only.
        float sum = 0; int n = 0;
        for (size_t i = 0; i < t->class_history.size(); ++i) {
            if (t->class_history[i] == p_.target) {
                sum += t->conf_history[i]; ++n;
            }
        }
        if (n == 0 || (sum / n) < p_.min_conf) continue;

        // Velocity from last 1 s of bbox history (or all if shorter).
        if (t->bbox_history.size() < 2) continue;
        size_t k = std::min<size_t>(t->bbox_history.size(), (size_t)std::max(1.0, fps));
        cv::Point a(t->bbox_history.front().x + t->bbox_history.front().width / 2,
                    t->bbox_history.front().y + t->bbox_history.front().height / 2);
        cv::Point b(t->bbox_history.back().x + t->bbox_history.back().width / 2,
                    t->bbox_history.back().y + t->bbox_history.back().height / 2);
        double dt_s = (double)(t->bbox_history.size() - 1) / fps;
        if (dt_s <= 0) continue;
        float vel = (float)(cv::norm(b - a) / dt_s);
        if (vel >= p_.max_velocity_pps) continue;

        cv::Point center(t->bbox.x + t->bbox.width / 2, t->bbox.y + t->bbox.height / 2);
        if (!inside(p_.strike_zone, center)) continue;

        dec.fire = true;
        dec.track_id = t->id;
        dec.cls = p_.target;
        dec.confidence = sum / n;
        dec.bbox = t->bbox;
        last_fire_us_ = now_us;
        break;
    }
    return dec;
}

}  // namespace hs::vision
