// firmware/vision/include/vision/frame_source.h
#pragma once
#include "vision/frame.h"

namespace hs::vision {

class IFrameSource {
public:
    virtual ~IFrameSource() = default;
    virtual bool open() = 0;
    virtual bool read(Frame& out) = 0;   // false on EOF or error
    virtual void close() = 0;
    virtual int width()  const = 0;
    virtual int height() const = 0;
    virtual double fps() const = 0;
};

}  // namespace hs::vision
