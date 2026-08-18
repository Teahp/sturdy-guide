#include "camera/opencv_camera.hpp"

#include <stdexcept>

namespace camera {
namespace {

int preferredBackend() {
#if defined(__linux__)
  return cv::CAP_V4L2;
#else
  return cv::CAP_ANY;
#endif
}

void configureCapture(cv::VideoCapture& capture, const int width,
                      const int height) {
#if defined(__linux__)
  capture.set(cv::CAP_PROP_FOURCC,
              cv::VideoWriter::fourcc('M', 'J', 'P', 'G'));
  capture.set(cv::CAP_PROP_BUFFERSIZE, 1.0);
#endif
  capture.set(cv::CAP_PROP_FRAME_WIDTH, static_cast<double>(width));
  capture.set(cv::CAP_PROP_FRAME_HEIGHT, static_cast<double>(height));
  capture.set(cv::CAP_PROP_FPS, 30.0);
}

}  // namespace

OpenCvCamera::OpenCvCamera(const int device_index, const int width,
                           const int height)
    : device_index_{device_index}, width_{width}, height_{height} {}

void OpenCvCamera::open() {
  close();
  capture_.open(device_index_, preferredBackend());
  if (!capture_.isOpened()) {
    throw std::runtime_error(
        "cannot open camera; check the device index, permissions, and "
        "whether another program is using it");
  }

  configureCapture(capture_, width_, height_);
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
