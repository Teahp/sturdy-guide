#pragma once
#include "camera/frame_source.hpp"
#include <opencv2/videoio.hpp>

namespace camera {

// 真实摄像头实现：基于cv::VideoCapture访问UVC设备
class OpenCvCamera : public FrameSource {
public:
    OpenCvCamera(int device_index, int width, int height);
    ~OpenCvCamera() override;

    void read(cv::Mat& frame) override;

private:
    cv::VideoCapture capture_;
};

} // namespace camera
