#include "../include/OpenCVCamera.hpp"
#include <stdexcept>

OpenCVCamera::OpenCVCamera(int deviceIndex) {
    cap_.open(deviceIndex, cv::CAP_ANY);
    if (!cap_.isOpened()) {
        throw std::runtime_error("Failed to open camera device: " + std::to_string(deviceIndex));
    }
}

OpenCVCamera::~OpenCVCamera() {
    if (cap_.isOpened()) {
        cap_.release();
    }
}

std::optional<cv::Mat> OpenCVCamera::getFrame() {
    cv::Mat frame;
    if (!cap_.read(frame) || frame.empty()) {
        return std::nullopt;
    }
    return frame;
}

bool OpenCVCamera::isOpened() const {
    return cap_.isOpened();
}
