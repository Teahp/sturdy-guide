#include "camera/opencv_camera.hpp"

#include <string>

namespace sturdy_guide::camera {

bool OpenCvCamera::open(int device, int width, int height) {
  camera_.release();
  camera_.open(device, cv::CAP_ANY);
  if (!camera_.isOpened()) {
    last_error_ =
        "cannot open camera; check the device index, permissions, and "
        "whether another program is using it";
    return false;
  }
  camera_.set(cv::CAP_PROP_FRAME_WIDTH, static_cast<double>(width));
  camera_.set(cv::CAP_PROP_FRAME_HEIGHT, static_cast<double>(height));
  produced_ = 0;
  return true;
}

bool OpenCvCamera::read(Frame& out) {
  if (!camera_.read(out.image)) {
    last_error_ = "camera stopped returning valid frames";
    return false;
  }
  out.sequence = produced_++;
  return true;
}

std::string OpenCvCamera::lastError() const { return last_error_; }

}  // namespace sturdy_guide::camera
