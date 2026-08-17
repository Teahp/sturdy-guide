#include "camera/opencv_camera.hpp"

#include <stdexcept>

namespace camera {

OpenCvCamera::OpenCvCamera(const int device_index, const int width,
                           const int height)
    : device_index_(device_index), width_(width), height_(height) {}

void OpenCvCamera::open() {
  video_ = cv::VideoCapture(device_index_, cv::CAP_ANY);
  if (!video_.isOpened()) {
    throw std::runtime_error(
        "cannot open camera; check the device index, permissions, and "
        "whether another program is using it");
  }
  video_.set(cv::CAP_PROP_FRAME_WIDTH, static_cast<double>(width_));
  video_.set(cv::CAP_PROP_FRAME_HEIGHT, static_cast<double>(height_));
}

bool OpenCvCamera::read(cv::Mat& frame) { return video_.read(frame); }

}  // namespace camera
