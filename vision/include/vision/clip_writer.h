// firmware/vision/include/vision/clip_writer.h
#pragma once
#include <opencv2/core.hpp>
#include <deque>
#include <string>
#include <cstdio>

namespace hs::vision {

class ClipWriter {
public:
    struct Params {
        int    pre_seconds = 5;
        int    post_seconds = 5;
        double fps = 30.0;
        std::string base_dir = "/var/lib/hornet-snapper/clips";
        std::string ffmpeg_bin = "ffmpeg";
    };
    explicit ClipWriter(Params p);
    ~ClipWriter();
    void push(const cv::Mat& bgr_frame);     // call every frame
    bool start_clip(const std::string& clip_id, int width, int height);
    void tick();                             // call every frame; closes when post window elapses
    bool is_recording() const { return state_ == State::Recording; }
    const std::string& current_path() const { return current_path_; }
private:
    enum class State { Idle, Recording, Closing };
    Params p_;
    std::deque<cv::Mat> ring_;
    State state_ = State::Idle;
    int post_frames_remaining_ = 0;
    int width_ = 0, height_ = 0;
    std::string current_path_;
    std::FILE* ffmpeg_ = nullptr;

    void open_ffmpeg(int w, int h);
    void close_ffmpeg();
    void write_frame(const cv::Mat& f);
};

}  // namespace hs::vision
