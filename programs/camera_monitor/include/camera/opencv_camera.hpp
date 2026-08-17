#pragma once

#include "camera/frame_source.hpp"

#include <opencv2/videoio.hpp>

namespace camera {

// 真实设备后端：封装 cv::VideoCapture。
// open() 在调用线程打开设备并设置请求的分辨率，read() 委托给 VideoCapture。
class OpenCvCamera final : public FrameSource {
 public:
  OpenCvCamera(int device_index, int width, int height);

  void open() override;
  bool read(cv::Mat& frame) override;
  int width() const override { return width_; }
  int height() const override { return height_; }

 private:
  int device_index_;
  int width_;
  int height_;
  cv::VideoCapture video_;
};

}  // namespace camera
