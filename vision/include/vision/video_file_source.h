// firmware/vision/include/vision/video_file_source.h
#pragma once
#include "vision/frame_source.h"
#include <opencv2/videoio.hpp>
#include <string>

namespace hs::vision {

class VideoFileSource : public IFrameSource {
public:
    explicit VideoFileSource(std::string path);
    ~VideoFileSource() override;
    bool open() override;
    bool read(Frame& out) override;
    void close() override;
    int width()  const override { return width_; }
    int height() const override { return height_; }
    double fps() const override { return fps_; }
private:
    std::string path_;
    cv::VideoCapture cap_;
    int width_ = 0, height_ = 0;
    double fps_ = 0.0;
    uint32_t seq_ = 0;
};

}  // namespace hs::vision
