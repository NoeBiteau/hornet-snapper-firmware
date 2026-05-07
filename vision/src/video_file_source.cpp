// firmware/vision/src/video_file_source.cpp
#include "vision/video_file_source.h"
#include <utility>

namespace hs::vision {

VideoFileSource::VideoFileSource(std::string path) : path_(std::move(path)) {}
VideoFileSource::~VideoFileSource() { close(); }

bool VideoFileSource::open() {
    if (!cap_.open(path_, cv::CAP_FFMPEG)) return false;
    width_  = (int)cap_.get(cv::CAP_PROP_FRAME_WIDTH);
    height_ = (int)cap_.get(cv::CAP_PROP_FRAME_HEIGHT);
    fps_    = cap_.get(cv::CAP_PROP_FPS);
    if (fps_ < 1.0) fps_ = 30.0;  // fall back if metadata missing
    return true;
}

bool VideoFileSource::read(Frame& out) {
    if (!cap_.read(out.bgr) || out.bgr.empty()) return false;
    out.ts_us = (uint64_t)((double)seq_ * 1e6 / fps_);
    out.seq = seq_++;
    return true;
}

void VideoFileSource::close() {
    if (cap_.isOpened()) cap_.release();
}

}  // namespace hs::vision
