// firmware/vision/src/clip_writer.cpp
#include "vision/clip_writer.h"
#include <opencv2/imgproc.hpp>
#include <filesystem>
#include <sstream>
#include <ctime>

namespace fs = std::filesystem;
namespace hs::vision {

ClipWriter::ClipWriter(Params p) : p_(std::move(p)) {}
ClipWriter::~ClipWriter() { close_ffmpeg(); }

void ClipWriter::push(const cv::Mat& f) {
    if (state_ == State::Idle) {
        ring_.push_back(f.clone());
        size_t cap = (size_t)(p_.pre_seconds * p_.fps);
        while (ring_.size() > cap) ring_.pop_front();
    } else if (state_ == State::Recording) {
        write_frame(f);
    }
}

bool ClipWriter::start_clip(const std::string& clip_id, int w, int h) {
    if (state_ != State::Idle) return false;
    auto today = []() {
        std::time_t t = std::time(nullptr);
        std::tm tm{}; localtime_r(&t, &tm);
        char buf[16]; std::strftime(buf, sizeof(buf), "%Y-%m-%d", &tm);
        return std::string(buf);
    };
    fs::path dir = fs::path(p_.base_dir) / today();
    fs::create_directories(dir);
    current_path_ = (dir / (clip_id + ".mp4")).string();
    width_ = w; height_ = h;
    open_ffmpeg(w, h);
    if (!ffmpeg_) return false;
    for (auto& f : ring_) write_frame(f);
    state_ = State::Recording;
    post_frames_remaining_ = (int)(p_.post_seconds * p_.fps);
    return true;
}

void ClipWriter::tick() {
    if (state_ != State::Recording) return;
    if (--post_frames_remaining_ <= 0) {
        close_ffmpeg();
        state_ = State::Idle;
        ring_.clear();
    }
}

void ClipWriter::open_ffmpeg(int w, int h) {
    std::ostringstream cmd;
    cmd << p_.ffmpeg_bin
        << " -y -loglevel error"
        << " -f rawvideo -pix_fmt bgr24"
        << " -s " << w << "x" << h
        << " -r " << p_.fps
        << " -i -"
        << " -c:v libx264 -preset veryfast -crf 23"
        << " -pix_fmt yuv420p"
        << " " << current_path_;
    ffmpeg_ = popen(cmd.str().c_str(), "w");
}

void ClipWriter::write_frame(const cv::Mat& f) {
    if (!ffmpeg_) return;
    cv::Mat fixed = f;
    if (f.cols != width_ || f.rows != height_) cv::resize(f, fixed, {width_, height_});
    if (!fixed.isContinuous()) fixed = fixed.clone();
    std::fwrite(fixed.data, 1, (size_t)fixed.total() * fixed.elemSize(), ffmpeg_);
}

void ClipWriter::close_ffmpeg() {
    if (!ffmpeg_) return;
    pclose(ffmpeg_);
    ffmpeg_ = nullptr;
}

}  // namespace hs::vision
