#include "camera/opencv_camera.hpp"
#include <chrono>

namespace camera {

OpenCvCamera::OpenCvCamera(int deviceIndex) {
    cap_.open(deviceIndex);
}

OpenCvCamera::~OpenCvCamera() {
    if (cap_.isOpened()) {
        cap_.release();
    }
}

std::optional<Frame> OpenCvCamera::read() {
    if (!cap_.isOpened()) {
        return std::nullopt;
    }
    cv::Mat frame;
    if (!cap_.read(frame) || frame.empty()) {
        return std::nullopt;
    }
    Frame f;
    f.image = frame.clone();
    f.frame_index = frame_counter_++;
    f.timestamp = std::chrono::duration<double>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    return f;
}

bool OpenCvCamera::isOpened() const {
    return cap_.isOpened();
}

double OpenCvCamera::getProperty(int propId) const {
    return cap_.get(propId);
}

bool OpenCvCamera::setProperty(int propId, double value) {
    return cap_.set(propId, value);
}

} // namespace camera
