#pragma once

#include <opencv2/core.hpp>
#include <optional>
#include <cstdint>

namespace camera {

struct Frame {
    cv::Mat image;
    int64_t frame_index = 0;
    double timestamp = 0.0;
};

class FrameSource {
public:
    virtual ~FrameSource() = default;

    virtual std::optional<Frame> read() = 0;
    virtual bool isOpened() const = 0;
    virtual double getProperty(int propId) const = 0;
    virtual bool setProperty(int propId, double value) = 0;
};

} // namespace camera
