#include "camera/opencv_camera.hpp"

#include <stdexcept>

namespace camera {

OpenCvCamera::OpenCvCamera(const int device_index, const int width,
                           const int height)
    : device_index_{device_index}, width_{width}, height_{height} {}

void OpenCvCamera::open() {
  close();
  capture_.open(device_index_, cv::CAP_ANY);
  if (!capture_.isOpened()) {
    throw std::runtime_error(
        "cannot open camera; check the device index, permissions, and "
        "whether another program is using it");
  }

  capture_.set(cv::CAP_PROP_FRAME_WIDTH, static_cast<double>(width_));
  capture_.set(cv::CAP_PROP_FRAME_HEIGHT, static_cast<double>(height_));
}

cv::Mat OpenCvCamera::read() {
  cv::Mat frame;
  if (!capture_.read(frame)) {
    throw std::runtime_error("camera stopped returning frames");
  }
  return frame;
}

void OpenCvCamera::close() {
  if (capture_.isOpened()) {
    capture_.release();
  }
}

}  // namespace camera
