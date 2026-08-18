#include "camera_monitor/opencv_camera.hpp"

#include <stdexcept>

namespace camera_monitor {

OpenCvCamera::OpenCvCamera(int device) : camera_(device) {
    if (!camera_.isOpened()) {
        throw std::runtime_error("failed to open camera device");
    }
}

cv::Mat OpenCvCamera::read() {
    cv::Mat frame;
    if (!camera_.read(frame)) {
        throw std::runtime_error("failed to read camera frame");
    }
    return frame;
}

}  // namespace camera_monitor
