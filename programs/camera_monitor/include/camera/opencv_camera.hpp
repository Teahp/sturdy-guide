#pragma once

#include "camera/frame_source.hpp"

#include <opencv2/videoio.hpp>

#include <cstdint>
#include <string>

namespace sturdy_guide::camera {

// 基于 cv::VideoCapture 的真实设备采集源。
class OpenCvCamera final : public FrameSource {
 public:
  bool open(int device, int width, int height) override;
  bool read(Frame& out) override;
  [[nodiscard]] std::string lastError() const override;

 private:
  cv::VideoCapture camera_;
  std::uint64_t produced_ = 0;
  std::string last_error_;
};

}  // namespace sturdy_guide::camera
