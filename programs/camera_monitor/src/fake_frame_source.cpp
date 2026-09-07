#include "camera/fake_frame_source.hpp"
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

namespace sturdy_guide::camera {

FakeFrameSource::FakeFrameSource(int w, int h) : width_(w), height_(h) {}

std::optional<cv::Mat> FakeFrameSource::read() {
    cv::Mat frame(height_, width_, CV_8UC3, cv::Scalar(0, 0, 0));
    // 画彩色渐变条纹
    for (int y = 0; y < height_; ++y) {
        uint8_t b = static_cast<uint8_t>((y * 255 / height_ + frame_counter_ * 2) % 256);
        uint8_t g = static_cast<uint8_t>((y * 255 / height_ + frame_counter_ * 3) % 256);
        uint8_t r = static_cast<uint8_t>((height_ - y) * 255 / height_);
        cv::line(frame, cv::Point(0, y), cv::Point(width_, y), cv::Scalar(b, g, r));
    }
    ++frame_counter_;
    cv::putText(frame, "Fake #" + std::to_string(frame_counter_),
                cv::Point(30, 50), cv::FONT_HERSHEY_SIMPLEX, 1.0,
                cv::Scalar(255, 255, 255), 2);
    cv::putText(frame, "Press S to save, Q to quit",
                cv::Point(30, height_ - 30), cv::FONT_HERSHEY_SIMPLEX, 0.6,
                cv::Scalar(200, 200, 200), 1);
    return frame;
}

} // namespace
