#pragma once

#include <opencv2/core.hpp>

namespace camera_monitor {

class FrameSource {
public:
    virtual ~FrameSource() = default;

    virtual cv::Mat read() = 0;
};

}  // namespace camera_monitor
