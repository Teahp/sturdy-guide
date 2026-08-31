#include "camera/fake_frame_source.hpp"
#include <opencv2/imgproc.hpp>
#include <stdexcept>
#include <sstream>
#include <thread>

namespace camera {

FakeFrameSource::FakeFrameSource(int width, int height, double fps)
    : width_(width), height_(height), fps_(fps),
      next_frame_time_(std::chrono::steady_clock::now()) {}

void FakeFrameSource::set_fail_after(size_t fail_after) {
    fail_after_ = fail_after;
}

void FakeFrameSource::read(cv::Mat& frame) {
    // 模拟真实摄像头的阻塞：等待到下一帧时间
    std::this_thread::sleep_until(next_frame_time_);
    auto frame_interval = std::chrono::duration<double>(1.0 / fps_);
    next_frame_time_ += std::chrono::duration_cast<std::chrono::steady_clock::duration>(frame_interval);

    if (fail_after_ > 0 && frame_count_ >= fail_after_) {
        throw std::runtime_error("simulated read failure");
    }

    frame = cv::Mat::zeros(height_, width_, CV_8UC3);
    frame.setTo(cv::Scalar(30, 30, 30));

    std::ostringstream text;
    text << "Frame " << frame_count_;
    cv::putText(frame, text.str(), cv::Point(20, height_ / 2),
                cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 255, 0), 2);

    ++frame_count_;
}

size_t FakeFrameSource::frame_count() const {
    return frame_count_;
}

} // namespace camera
