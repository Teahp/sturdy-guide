#pragma once

#include <opencv2/core.hpp>

namespace sturdy_guide::camera {

class FrameSource {
public:
    virtual ~FrameSource() = default;
    virtual cv::Mat read() = 0;
};

} // namespace sturdy_guide::camera