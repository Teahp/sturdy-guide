#pragma once
#include "frame_source.hpp"
#include <opencv2/videoio.hpp>

namespace sturdy_guide::camera {

// 真相机：使用 OpenCV 调用真实摄像头
class OpenCvCamera : public FrameSource {
public:
    OpenCvCamera(int device_index, int width = 640, int height = 480);
    std::optional<cv::Mat> read() override;
    int width() const override { return width_; }
    int height() const override { return height_; }

private:
    cv::VideoCapture cap_;
    int width_;
    int height_;
};

} // namespace
