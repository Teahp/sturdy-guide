#pragma once

#include "camera/frame_source.hpp"

#include <opencv2/videoio.hpp>

namespace camera {

// Real-device frame source backed by cv::VideoCapture.
class OpenCvCamera final : public FrameSource {
 public:
  // Opens the given device and requests the given resolution.
  // Throws std::runtime_error when the device cannot be opened.
  OpenCvCamera(int device_index, int width, int height);

  bool read(cv::Mat& frame) override;

 private:
  cv::VideoCapture capture_;
};

}  // namespace camera
