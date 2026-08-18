#include "opencv_camera.h"
#include <stdexcept>

OpenCvCamera::OpenCvCamera(int device, int width, int height)
    : device_(device), width_(width), height_(height) {}

bool OpenCvCamera::open() {
    if (opened_) return true;
    cap_.open(device_);
    if (!cap_.isOpened()) return false;
    cap_.set(cv::CAP_PROP_FRAME_WIDTH, width_);
    cap_.set(cv::CAP_PROP_FRAME_HEIGHT, height_);
    opened_ = true;
    return true;
}

std::optional<cv::Mat> OpenCvCamera::read() {
    if (!opened_) return std::nullopt;
    cv::Mat frame;
    if (!cap_.read(frame) || frame.empty()) {
        return std::nullopt;
    }
    return frame;
}

void OpenCvCamera::close() {
    if (opened_) {
        cap_.release();
        opened_ = false;
    }
}

bool OpenCvCamera::isOpen() const {
    return opened_ && cap_.isOpened();
}
