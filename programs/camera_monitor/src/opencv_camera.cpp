#include "opencv_camera.h"
#include <opencv2/videoio.hpp>
#include <sstream>
#include <stdexcept>

namespace camera {

OpenCvCamera::OpenCvCamera(int device_index, int width, int height)
    : device_index_(device_index),
      requested_width_(width),
      requested_height_(height) {}

void OpenCvCamera::open() {
    if (capture_.isOpened()) {
        return;  // 已打开
    }

    capture_.open(device_index_, cv::CAP_ANY);
    if (!capture_.isOpened()) {
        throw std::runtime_error(
            "cannot open camera " + std::to_string(device_index_) +
            "; check the device index, permissions, and whether another "
            "program is using it");
    }

    // 设置分辨率（尽力而为，实际分辨率可能不同）
    capture_.set(cv::CAP_PROP_FRAME_WIDTH, static_cast<double>(requested_width_));
    capture_.set(cv::CAP_PROP_FRAME_HEIGHT, static_cast<double>(requested_height_));
}

bool OpenCvCamera::read(Frame& frame) {
    if (!capture_.isOpened()) {
        return false;
    }

    cv::Mat image;
    if (!capture_.read(image) || image.empty()) {
        return false;
    }

    frame.image = image.clone();  // 复制以确保线程安全
    frame.valid = true;
    frame.timestamp = std::chrono::steady_clock::now();
    return true;
}

void OpenCvCamera::close() {
    if (capture_.isOpened()) {
        capture_.release();
    }
}

bool OpenCvCamera::isOpened() const {
    return capture_.isOpened();
}

std::string OpenCvCamera::description() const {
    std::ostringstream oss;
    oss << "OpenCV camera (device " << device_index_ << ", "
        << requested_width_ << "x" << requested_height_ << ")";
    return oss.str();
}

}  // namespace camera
