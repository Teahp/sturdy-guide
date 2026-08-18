#pragma once

#include "camera/frame_source.hpp"

#include <opencv2/videoio.hpp>

namespace camera {

class OpenCvCamera final : public FrameSource {
 public:
  OpenCvCamera(int device_index, int width, int height);

  void open() override;
  cv::Mat read() override;
  void close() override;

 private:
  int device_index_;
  int width_;
  int height_;
  cv::VideoCapture capture_;
};

}  // namespace camera
