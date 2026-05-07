// firmware/apps/rv1106/main.cpp
#ifdef HS_HAS_LIBCAMERA
#include "vision/libcamera_source.h"
#endif
#include "vision/video_file_source.h"
#include "vision/motion_gate.h"
#include "vision/mock_detector.h"
#include "vision/onnx_detector.h"
#include "vision/tracker.h"
#include "vision/confirmer.h"
#include "vision/clip_writer.h"
#include "vision/event_log.h"
#include "hal/time.h"

#include <nlohmann/json.hpp>

#include <cstdio>
#include <cstring>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

using namespace hs::vision;

namespace {
struct Args {
    std::string video, model, data_dir = "/var/lib/hornet-snapper", zone;
    bool live = false;
    int max_frames = -1;
    int confirm_min_age_frames = -1;
    int confirm_class_min_match = -1;
    uint64_t cooldown_us = UINT64_MAX;
    double max_velocity_pps = -1.0;
};

bool parse(int argc, char** argv, Args& a) {
    for (int i = 1; i < argc; ++i) {
        std::string k = argv[i];
        auto next = [&](std::string& out){ if (++i >= argc) return false; out = argv[i]; return true; };
        if      (k == "--video")        { if(!next(a.video)) return false; }
        else if (k == "--live")         { a.live = true; }
        else if (k == "--model")        { if(!next(a.model)) return false; }
        else if (k == "--data-dir")     { if(!next(a.data_dir)) return false; }
        else if (k == "--strike-zone")  { if(!next(a.zone)) return false; }
        else if (k == "--max-frames")   { std::string v; if(!next(v)) return false; a.max_frames = std::stoi(v); }
        else if (k == "--confirm-min-age-frames") { std::string v; if(!next(v)) return false; a.confirm_min_age_frames = std::stoi(v); }
        else if (k == "--confirm-class-min-match") { std::string v; if(!next(v)) return false; a.confirm_class_min_match = std::stoi(v); }
        else if (k == "--cooldown-us") { std::string v; if(!next(v)) return false; a.cooldown_us = static_cast<uint64_t>(std::stoull(v)); }
        else if (k == "--max-velocity-pps") { std::string v; if(!next(v)) return false; a.max_velocity_pps = std::stod(v); }
        else { std::fprintf(stderr, "unknown arg: %s\n", k.c_str()); return false; }
    }
    if (a.video.empty() && !a.live) { std::fprintf(stderr, "either --video or --live required\n"); return false; }
    return true;
}

std::vector<cv::Point> parse_zone(const std::string& s) {
    std::vector<cv::Point> out;
    if (s.empty()) return out;
    std::stringstream ss(s); std::string tok;
    while (std::getline(ss, tok, ';')) {
        size_t c = tok.find(',');
        if (c == std::string::npos) continue;
        out.emplace_back(std::stoi(tok.substr(0, c)), std::stoi(tok.substr(c + 1)));
    }
    return out;
}
}  // namespace

int main(int argc, char** argv) {
    Args a;
    if (!parse(argc, argv, a)) return 2;
    std::printf("hornet_snapper_rv1106 starting (video=%s model=%s data_dir=%s)\n",
                a.video.c_str(), a.model.c_str(), a.data_dir.c_str());

    std::unique_ptr<IFrameSource> src;
    if (a.live) {
#ifdef HS_HAS_LIBCAMERA
        src = std::make_unique<LibcameraSource>();
#else
        std::fprintf(stderr, "--live requires HS_VISION_LIBCAMERA=ON build\n");
        return 3;
#endif
    } else {
        src = std::make_unique<VideoFileSource>(a.video);
    }
    if (!src->open()) { std::fprintf(stderr, "frame source open failed\n"); return 4; }

    MotionGate gate(src->width(), src->height());
    Tracker    tracker;

    // Load a mock script from a JSON file at mock://path
    auto load_mock_script = [](const std::string& uri)
        -> std::vector<std::vector<Detection>>
    {
        std::string path = uri.substr(7); // strip "mock://"
        std::ifstream f(path);
        if (!f) {
            std::fprintf(stderr, "mock script not found: %s\n", path.c_str());
            return {};
        }
        auto j = nlohmann::json::parse(f, nullptr, /*exceptions=*/false);
        if (!j.is_array()) {
            std::fprintf(stderr, "mock script must be a JSON array: %s\n", path.c_str());
            return {};
        }
        std::vector<std::vector<Detection>> script;
        for (auto& frame : j) {
            std::vector<Detection> dets;
            if (frame.is_array()) {
                for (auto& d : frame) {
                    Detection det;
                    det.bbox = cv::Rect(d["x"].get<int>(), d["y"].get<int>(),
                                       d["w"].get<int>(), d["h"].get<int>());
                    det.confidence = d["conf"].get<float>();
                    det.cls = static_cast<ClassId>(d["cls"].get<int>());
                    dets.push_back(det);
                }
            }
            script.push_back(std::move(dets));
        }
        return script;
    };

    std::unique_ptr<IDetector> det;
    if (a.model.substr(0, 7) == "mock://") {
        det = std::make_unique<MockDetector>(load_mock_script(a.model));
    } else if (!a.model.empty()) {
        det = std::make_unique<OnnxDetector>(a.model);
    } else {
        det = std::make_unique<MockDetector>(std::vector<std::vector<Detection>>{});
    }
    if (!det->init()) { std::fprintf(stderr, "detector init failed\n"); return 5; }

    Confirmer::Params cp;
    cp.strike_zone = parse_zone(a.zone);
    if (a.confirm_min_age_frames >= 0) cp.min_age_frames = a.confirm_min_age_frames;
    if (a.confirm_class_min_match >= 0) cp.class_min_match = a.confirm_class_min_match;
    if (a.cooldown_us != UINT64_MAX) cp.cooldown_us = a.cooldown_us;
    if (a.max_velocity_pps >= 0.0) cp.max_velocity_pps = static_cast<float>(a.max_velocity_pps);
    Confirmer confirmer(cp);

    ClipWriter::Params cwp;
    cwp.fps = src->fps(); cwp.base_dir = a.data_dir + "/clips";
    ClipWriter clipw(std::move(cwp));

    EventLog evlog(a.data_dir + "/events.jsonl");
    if (!evlog.open()) { std::fprintf(stderr, "event log open failed\n"); return 6; }

    Frame f;
    int frame_idx = 0;
    while (src->read(f)) {
        clipw.push(f.bgr);
        clipw.tick();

        auto rois = gate.process(f);
        std::vector<cv::Rect> roi_rects;
        for (auto& r : rois) roi_rects.push_back(r.bbox);

        bool nn_ran = !roi_rects.empty() && (frame_idx % 6 == 0);
        std::vector<Detection> dets;
        if (nn_ran) dets = det->infer(f.bgr, roi_rects);
        tracker.update(f.bgr, dets, nn_ran);

        auto dec = confirmer.evaluate(tracker.tracks(), src->fps(), f.ts_us);
        if (dec.fire) {
            std::string clip_id = "ev_" + std::to_string(f.ts_us);
            clipw.start_clip(clip_id, f.bgr.cols, f.bgr.rows);
            evlog.write(dec, f.ts_us, clipw.current_path());
            std::printf("FIRE track=%u cls=%d conf=%.2f bbox=[%d,%d,%d,%d] clip=%s\n",
                        dec.track_id, (int)dec.cls, dec.confidence,
                        dec.bbox.x, dec.bbox.y, dec.bbox.width, dec.bbox.height,
                        clipw.current_path().c_str());
        }
        ++frame_idx;
        if (a.max_frames > 0 && frame_idx >= a.max_frames) break;
    }

    src->close();
    evlog.close();
    return 0;
}
