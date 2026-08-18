#include "camera/opencv_camera.hpp"

#include <stdexcept>

namespace sturdy_guide::camera {

OpenCvCamera::OpenCvCamera(const int device, const int width, const int height)
    : camera_(device, cv::CAP_V4L2) {
  if (device < 0 || width <= 0 || height <= 0) {
    throw std::invalid_argument(
        "device must be non-negative and dimensions must be positive");
  }
  if (!camera_.isOpened()) {
    throw std::runtime_error(
        "cannot open camera; check the device index, permissions, and "
        "whether another program is using it");
  }

  camera_.set(cv::CAP_PROP_FRAME_WIDTH, static_cast<double>(width));
  camera_.set(cv::CAP_PROP_FRAME_HEIGHT, static_cast<double>(height));
}

cv::Mat OpenCvCamera::read() {
  if (stop_requested_.load()) {
    return {};
  }

  cv::Mat frame;
  if (!camera_.read(frame)) {
    if (stop_requested_.load()) {
      return {};
    }
    throw std::runtime_error("camera stopped returning valid frames");
  }
  return frame;
}

void OpenCvCamera::request_stop() noexcept {
  stop_requested_.store(true);//设置 stop_requested_ 标志为 true，表示请求停止摄像头读取操作。
  try {
    // 释放相机资源
    camera_.release();
  } catch (...) {
    // 捕获所有异常，但不会抛出异常
  }
}

}  // namespace sturdy_guide::camera
