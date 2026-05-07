// firmware/vision/src/libcamera_source.cpp
// LibcameraSource implementation — Pi5 live capture via libcamera C++ API.
// Only compiled when HS_VISION_LIBCAMERA=ON (defines HS_HAS_LIBCAMERA).
#ifdef HS_HAS_LIBCAMERA

#include "vision/libcamera_source.h"

#include <libcamera/libcamera.h>

#include <sys/mman.h>

#include <chrono>
#include <cstdio>
#include <cstring>

namespace hs::vision {

LibcameraSource::LibcameraSource(int width, int height, double fps)
    : width_(width), height_(height), fps_(fps) {}

LibcameraSource::~LibcameraSource() { close(); }

bool LibcameraSource::open() {
    // --- Camera manager ---
    cam_mgr_ = std::make_unique<libcamera::CameraManager>();
    if (cam_mgr_->start() != 0) {
        std::fprintf(stderr, "LibcameraSource: CameraManager::start() failed\n");
        return false;
    }

    auto cameras = cam_mgr_->cameras();
    if (cameras.empty()) {
        std::fprintf(stderr, "LibcameraSource: no cameras found\n");
        return false;
    }
    camera_ = cameras[0];

    if (camera_->acquire() != 0) {
        std::fprintf(stderr, "LibcameraSource: camera acquire failed\n");
        return false;
    }

    // --- Configure stream: BGR888, requested resolution ---
    config_ = camera_->generateConfiguration({libcamera::StreamRole::Viewfinder});
    if (!config_) {
        std::fprintf(stderr, "LibcameraSource: generateConfiguration failed\n");
        return false;
    }

    libcamera::StreamConfiguration& scfg = config_->at(0);
    scfg.pixelFormat = libcamera::formats::BGR888;
    scfg.size        = {(unsigned int)width_, (unsigned int)height_};
    scfg.bufferCount = 4;

    auto status = config_->validate();
    if (status == libcamera::CameraConfiguration::Invalid) {
        std::fprintf(stderr, "LibcameraSource: stream configuration invalid\n");
        return false;
    }
    // Update actual dimensions after validation (camera may adjust them).
    width_  = (int)scfg.size.width;
    height_ = (int)scfg.size.height;

    if (camera_->configure(config_.get()) != 0) {
        std::fprintf(stderr, "LibcameraSource: camera configure failed\n");
        return false;
    }

    // --- Allocate buffers ---
    libcamera::Stream* stream = scfg.stream();
    allocator_ = std::make_unique<libcamera::FrameBufferAllocator>(camera_);
    if (allocator_->allocate(stream) < 0) {
        std::fprintf(stderr, "LibcameraSource: buffer allocation failed\n");
        return false;
    }

    // --- Build requests ---
    const auto& buffers = allocator_->buffers(stream);
    for (const auto& buf : buffers) {
        auto req = camera_->createRequest();
        if (!req) {
            std::fprintf(stderr, "LibcameraSource: createRequest failed\n");
            return false;
        }
        if (req->addBuffer(stream, buf.get()) != 0) {
            std::fprintf(stderr, "LibcameraSource: addBuffer failed\n");
            return false;
        }
        requests_.push_back(std::move(req));
    }

    // --- Connect completion signal ---
    camera_->requestCompleted.connect(this, &LibcameraSource::requestComplete);

    // --- Start camera ---
    if (camera_->start() != 0) {
        std::fprintf(stderr, "LibcameraSource: camera start failed\n");
        return false;
    }
    running_ = true;

    // Queue all requests.
    for (auto& req : requests_) {
        if (camera_->queueRequest(req.get()) != 0) {
            std::fprintf(stderr, "LibcameraSource: queueRequest failed\n");
            running_ = false;
            camera_->stop();
            return false;
        }
    }

    return true;
}

// Called from libcamera's internal thread — must be signal-safe.
void LibcameraSource::requestComplete(libcamera::Request* req) {
    if (req->status() == libcamera::Request::RequestCancelled) return;
    {
        std::lock_guard<std::mutex> lk(mtx_);
        completed_.push(req);
    }
    cv_.notify_one();
}

bool LibcameraSource::read(Frame& out) {
    if (!running_) return false;

    libcamera::Request* req = nullptr;
    {
        std::unique_lock<std::mutex> lk(mtx_);
        cv_.wait_for(lk, std::chrono::milliseconds(2000),
                     [this]{ return !completed_.empty() || !running_; });
        if (completed_.empty()) return false;
        req = completed_.front();
        completed_.pop();
    }

    // Map the first plane and copy into a cv::Mat.
    libcamera::Stream* stream = config_->at(0).stream();
    libcamera::FrameBuffer* fb = req->findBuffer(stream);
    if (!fb) {
        std::fprintf(stderr, "LibcameraSource: no buffer in request\n");
        return false;
    }

    const libcamera::FrameBuffer::Plane& plane = fb->planes()[0];
    size_t expected = (size_t)width_ * (size_t)height_ * 3;
    size_t len = std::min((size_t)plane.length, expected);

    void* mem = mmap(nullptr, len, PROT_READ, MAP_SHARED, plane.fd.get(), plane.offset);
    if (mem == MAP_FAILED) {
        std::fprintf(stderr, "LibcameraSource: mmap failed\n");
        return false;
    }

    out.bgr = cv::Mat(height_, width_, CV_8UC3);
    std::memcpy(out.bgr.data, mem, len);
    munmap(mem, len);

    // Timestamp from libcamera metadata (nanoseconds -> microseconds).
    const libcamera::ControlList& meta = req->metadata();
    auto ts_ctrl = meta.get(libcamera::controls::SensorTimestamp);
    if (ts_ctrl) {
        out.ts_us = (uint64_t)(*ts_ctrl / 1000);
    } else {
        // Fall back to sequence-based synthetic timestamp.
        out.ts_us = (uint64_t)((double)seq_ * 1e6 / fps_);
    }
    out.seq = seq_++;

    // Recycle the request.
    req->reuse(libcamera::Request::ReuseBuffers);
    {
        std::lock_guard<std::mutex> lk(mtx_);
    }
    camera_->queueRequest(req);

    return true;
}

void LibcameraSource::close() {
    if (!running_.exchange(false)) return;

    camera_->stop();
    camera_->requestCompleted.disconnect(this, &LibcameraSource::requestComplete);

    requests_.clear();
    if (allocator_) {
        libcamera::Stream* stream = config_->at(0).stream();
        allocator_->free(stream);
        allocator_.reset();
    }
    config_.reset();

    camera_->release();
    camera_.reset();

    cam_mgr_->stop();
    cam_mgr_.reset();
}

}  // namespace hs::vision

#endif  // HS_HAS_LIBCAMERA
