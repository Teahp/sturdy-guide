#pragma once

#include "camera_monitor/frame_source.hpp"

#include <opencv2/videoio.hpp>

namespace camera_monitor {

class OpenCvCamera final : public FrameSource {
public:
    explicit OpenCvCamera(int device);

    OpenCvCamera(const OpenCvCamera&) = delete;
    OpenCvCamera& operator=(const OpenCvCamera&) = delete;

    cv::Mat read() override;

private:
    cv::VideoCapture camera_;
};

}  // namespace camera_monitor
