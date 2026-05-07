// firmware/tools/gen_synth_video/main.cpp
#include <opencv2/opencv.hpp>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "usage: gen_synth_video <out.mp4>\n";
        return 1;
    }
    const std::string out = argv[1];
    const int W = 640, H = 480, FPS = 30;
    cv::VideoWriter vw(out, cv::VideoWriter::fourcc('m','p','4','v'),
                       FPS, cv::Size(W, H));
    if (!vw.isOpened()) { std::cerr << "writer open failed\n"; return 2; }

    cv::Mat bg(H, W, CV_8UC3, cv::Scalar(50, 60, 70));
    for (int i = 0; i < 60; ++i) {
        cv::Mat frame = bg.clone();
        if (i >= 10 && i < 50) {
            int x = 50 + (i - 10) * 8;
            cv::rectangle(frame, cv::Rect(x, 200, 40, 40),
                          cv::Scalar(255, 255, 255), cv::FILLED);
        }
        vw.write(frame);
    }
    return 0;
}
