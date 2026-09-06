#pragma once

#include "camera/frame_source.hpp"

#include <opencv2/videoio.hpp>

namespace sturdy_guide::camera {

// 真插头：通过 OpenCV 访问真实摄像头。实现 FrameSource 的 read()。
class OpenCvCamera final : public FrameSource {
 public:
  // 打开 device_index 号设备并请求 width x height 分辨率；失败抛异常。
  OpenCvCamera(int device_index, int width, int height);

  cv::Mat read() override;

 private:
  cv::VideoCapture capture_;
};

}  // namespace sturdy_guide::camera
