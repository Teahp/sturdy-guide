#include "camera/open_cv_camera.hpp"
#include <stdexcept>

namespace sturdy_guide::camera {

OpenCvCamera::OpenCvCamera(int index, int w, int h) : width_(w), height_(h) {
    cap_.open(index);
    if (!cap_.isOpened()) {
        throw std::runtime_error("无法打开摄像头 " + std::to_string(index));
    }
    cap_.set(cv::CAP_PROP_FRAME_WIDTH, static_cast<double>(w));
    cap_.set(cv::CAP_PROP_FRAME_HEIGHT, static_cast<double>(h));
    width_ = static_cast<int>(cap_.get(cv::CAP_PROP_FRAME_WIDTH));
    height_ = static_cast<int>(cap_.get(cv::CAP_PROP_FRAME_HEIGHT));
}

std::optional<cv::Mat> OpenCvCamera::read() {
    cv::Mat frame;
    if (cap_.read(frame) && !frame.empty()) {
        return frame;
    }
    return std::nullopt;
}

} // namespace
