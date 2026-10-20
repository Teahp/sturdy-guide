#pragma once

#include "camera/frame_source.hpp"

#include <string>

#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>

namespace camera {

// 使用 cv::VideoCapture 访问真实设备：Linux 上是 /dev/videoN，也支持 UVC USB 摄像头。
class OpenCvCamera final : public FrameSource {
 public:
  struct Settings {
    int device = 0;
    int width = 1280;
    int height = 720;
  };

  explicit OpenCvCamera(Settings settings);

  [[nodiscard]] bool open() override;
  void close() noexcept override;
  [[nodiscard]] bool is_open() const noexcept override;
  bool read(cv::Mat& frame) override;
  [[nodiscard]] std::string description() const override;

 private:
  Settings settings_;
  cv::VideoCapture capture_;
};

}  // namespace camera
