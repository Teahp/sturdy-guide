#include "camera/opencv_camera.h"

namespace camera {

OpenCvCamera::OpenCvCamera(int device_index, int width, int height)
    : device_index_(device_index), width_(width), height_(height) {}

bool OpenCvCamera::open() {
  release();
  if (!capture_.open(device_index_, cv::CAP_ANY)) {
    return false;
  }
  capture_.set(cv::CAP_PROP_FRAME_WIDTH, static_cast<double>(width_));
  capture_.set(cv::CAP_PROP_FRAME_HEIGHT, static_cast<double>(height_));
  return true;
}

bool OpenCvCamera::read(cv::Mat& frame) { return capture_.read(frame); }

void OpenCvCamera::release() {
  if (capture_.isOpened()) {
    capture_.release();
  }
}

}  // namespace camera
