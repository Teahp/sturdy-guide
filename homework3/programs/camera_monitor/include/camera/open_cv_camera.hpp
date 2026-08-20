#pragma once

#include "camera/frame_source.hpp"

#include <opencv2/videoio.hpp>

namespace sturdy_guide::camera {

// 通过 cv::VideoCapture 访问真实摄像头设备。
class OpenCvCamera final : public FrameSource {
 public:
  // device 为系统识别的设备编号；width/height 为期望分辨率。
  // 设备无法打开或参数非法时抛出 std::runtime_error / std::invalid_argument。
  OpenCvCamera(int device, int width, int height);

  cv::Mat read() override;

 private:
  cv::VideoCapture capture_;
};

}  // namespace sturdy_guide::camera
