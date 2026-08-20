#include "camera/open_cv_camera.hpp"

#include <stdexcept>

namespace sturdy_guide::camera {

OpenCvCamera::OpenCvCamera(const int device, const int width,
                           const int height)
    // Linux 上 USB 摄像头通过 V4L2 访问更稳定；GStreamer 后端对部分设备
    // 会报 "Internal data stream error"。
    : capture_{device, cv::CAP_V4L2} {
  if (device < 0 || width <= 0 || height <= 0) {
    throw std::invalid_argument(
        "device must be non-negative and dimensions must be positive");
  }
  if (!capture_.isOpened()) {
    throw std::runtime_error(
        "cannot open camera; check the device index, permissions, and "
        "whether another program is using it");
  }
  capture_.set(cv::CAP_PROP_FRAME_WIDTH, static_cast<double>(width));
  capture_.set(cv::CAP_PROP_FRAME_HEIGHT, static_cast<double>(height));
}

cv::Mat OpenCvCamera::read() {
  cv::Mat frame;
  if (!capture_.read(frame) || frame.empty()) {
    // 读不到有效帧视为设备故障，抛出后由 CameraSession 传回主线程报告。
    throw std::runtime_error("camera stopped returning valid frames");
  }
  return frame;
}

}  // namespace sturdy_guide::camera
