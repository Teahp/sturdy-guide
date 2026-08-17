#include "camera/fake_frame_source.hpp"
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>
#include <chrono>
#include <random>
#include <stdexcept>

namespace camera {

FakeFrameSource::FakeFrameSource(int width, int height, int fps)
    : width_(width), height_(height), fps_(fps),
      timestamp_base_(std::chrono::duration<double>(
          std::chrono::system_clock::now().time_since_epoch()
      ).count()) {}

std::optional<Frame> FakeFrameSource::read() {
    if (shouldFail_.load()) {
        throw std::runtime_error("FakeFrameSource: simulated read failure");
    }
    cv::Mat frame(height_, width_, CV_8UC3);
    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < width_; ++x) {
            uint8_t r = static_cast<uint8_t>((x * 255) / width_);
            uint8_t g = static_cast<uint8_t>((y * 255) / height_);
            uint8_t b = static_cast<uint8_t>(((x + y) * 255) / (width_ + height_));
            frame.at<cv::Vec3b>(y, x) = cv::Vec3b(b, g, r);
        }
    }
    std::string text = "Fake Frame " + std::to_string(frame_counter_);
    cv::putText(frame, text, cv::Point(30, 60), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 2);

    Frame f;
    f.image = frame;
    f.frame_index = frame_counter_++;
    f.timestamp = timestamp_base_ + static_cast<double>(f.frame_index) / fps_;
    return f;
}

double FakeFrameSource::getProperty(int propId) const {
    switch (propId) {
        case cv::CAP_PROP_FRAME_WIDTH: return width_;
        case cv::CAP_PROP_FRAME_HEIGHT: return height_;
        case cv::CAP_PROP_FPS: return fps_;
        default: return 0.0;
    }
}

bool FakeFrameSource::setProperty(int propId, double value) {
    return false;
}

} // namespace camera
