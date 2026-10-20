#include "camera/opencv_camera.hpp"

namespace camera {

OpenCvCamera::OpenCvCamera(Settings settings) : settings_(settings) {}

bool OpenCvCamera::open() {
  if (capture_.isOpened()) {
    return true;
  }

  capture_.open(settings_.device, cv::CAP_ANY);
  if (!capture_.isOpened()) {
    return false;
  }

  // 分辨率只作为请求，设备可能给出最接近的可用格式；
  // 真实生效的分辨率以读到的帧尺寸为准。
  capture_.set(cv::CAP_PROP_FRAME_WIDTH, static_cast<double>(settings_.width));
  capture_.set(cv::CAP_PROP_FRAME_HEIGHT, static_cast<double>(settings_.height));
  return true;
}

void OpenCvCamera::close() noexcept { capture_.release(); }

bool OpenCvCamera::is_open() const noexcept { return capture_.isOpened(); }

bool OpenCvCamera::read(cv::Mat& frame) {
  if (!capture_.isOpened()) {
    return false;
  }
  // 返回 true 但 frame 为空也是可能的；CameraSession 会把它计为丢弃，不会发布。
  return capture_.read(frame);
}

std::string OpenCvCamera::description() const {
  return "OpenCV device " + std::to_string(settings_.device) + " at " +
         std::to_string(settings_.width) + "x" +
         std::to_string(settings_.height);
}

}  // namespace camera
