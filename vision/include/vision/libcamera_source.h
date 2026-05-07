// firmware/vision/include/vision/libcamera_source.h
// LibcameraSource: Pi5 live capture via libcamera C++ API.
// Only compiled when HS_VISION_LIBCAMERA=ON (defines HS_HAS_LIBCAMERA).
#pragma once

#ifdef HS_HAS_LIBCAMERA

#include "vision/frame_source.h"

#include <libcamera/libcamera.h>

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>

namespace hs::vision {

// LibcameraSource pulls BGR frames from the first camera exposed by libcamera.
// Thread safety: open/close must be called from the same thread; read() may
// block briefly while waiting for the next completed request.
class LibcameraSource : public IFrameSource {
public:
    // width/height/fps are requested; actual values filled after open().
    explicit LibcameraSource(int width = 1920, int height = 1080, double fps = 30.0);
    ~LibcameraSource() override;

    bool open() override;
    bool read(Frame& out) override;
    void close() override;

    int    width()  const override { return width_; }
    int    height() const override { return height_; }
    double fps()    const override { return fps_; }

private:
    void requestComplete(libcamera::Request* req);

    int    width_;
    int    height_;
    double fps_;

    std::unique_ptr<libcamera::CameraManager>  cam_mgr_;
    std::shared_ptr<libcamera::Camera>         camera_;
    std::unique_ptr<libcamera::CameraConfiguration> config_;
    std::unique_ptr<libcamera::FrameBufferAllocator>  allocator_;
    std::vector<std::unique_ptr<libcamera::Request>>  requests_;

    // Completed requests passed from libcamera callback to read().
    std::mutex              mtx_;
    std::condition_variable cv_;
    std::queue<libcamera::Request*> completed_;
    std::atomic<bool> running_{false};

    uint32_t seq_ = 0;
};

}  // namespace hs::vision

#endif  // HS_HAS_LIBCAMERA
