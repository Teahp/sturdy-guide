#pragma once

#include "camera/frame_source.hpp"

#include <opencv2/videoio.hpp>

#include <string>

namespace sturdy_guide::camera {

// OpenCvCamera wraps a cv::VideoCapture device behind the FrameSource
// interface. It owns the hardware resource exclusively.
class OpenCvCamera final : public FrameSource {
 public:
  // Opens the device at the given index with the requested resolution.
  // Throws std::runtime_error if the device cannot be opened.
  OpenCvCamera(int device, int width, int height);

  [[nodiscard]] std::optional<cv::Mat> read() override;

 private:
  cv::VideoCapture capture_;
};

}  // namespace sturdy_guide::camera
//真实摄像头封装，继承 FrameSource