// 使用 cv::VideoCapture 访问真实摄像头的帧源。
// 构造时不打开设备；open() 时才建立连接，避免对象创建即占用硬件。
#pragma once

#include "camera/frame_source.h"

#include <opencv2/videoio.hpp>

namespace camera {

class OpenCvCamera : public FrameSource {
 public:
  OpenCvCamera(int device_index, int width = 1280, int height = 720);

  bool open() override;
  bool read(cv::Mat& frame) override;
  void release() override;

 private:
  int device_index_;
  int width_;
  int height_;
  cv::VideoCapture capture_;
};

}  // namespace camera
