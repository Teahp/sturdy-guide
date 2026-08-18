#include "camera/opencv_camera.hpp"

namespace camera {

OpenCvCamera::OpenCvCamera(int device, int width, int height)
    : device_(device), width_(width), height_(height) {}

bool OpenCvCamera::open() {
  if (capture_.isOpened()) {
    return true;
  }
  capture_.open(device_, cv::CAP_ANY);
  if (!capture_.isOpened()) {
    return false;
  }
  capture_.set(cv::CAP_PROP_FRAME_WIDTH, static_cast<double>(width_));
  capture_.set(cv::CAP_PROP_FRAME_HEIGHT, static_cast<double>(height_));
  return true;
}

bool OpenCvCamera::read(cv::Mat& frame) {
  return capture_.read(frame);
}

void OpenCvCamera::close() {
  capture_.release();
}

}  // namespace camera
