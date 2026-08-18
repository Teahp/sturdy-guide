#include "fake_frame_source.h"
#include <chrono>
#include <thread>
#include <stdexcept>

FakeFrameSource::FakeFrameSource(FailureMode mode) : mode_(mode) {}

bool FakeFrameSource::open() {
    if (mode_ == FailureMode::Stop) {
        opened_ = false;
        return false;
    }
    opened_ = true;
    return true;
}

std::optional<cv::Mat> FakeFrameSource::read() {
    if (!opened_) return std::nullopt;

    if (mode_ == FailureMode::Overflow) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if (mode_ == FailureMode::ReadException) {
        throw std::runtime_error("Simulated read exception");
    }

    if (mode_ == FailureMode::EmptyFrame) {
        return std::nullopt;
    }

    cv::Mat frame(480, 640, CV_8UC3, cv::Scalar(0, 0, 0));
    cv::putText(frame, "FAKE: " + std::to_string(++frame_counter_),
                cv::Point(50, 240), cv::FONT_HERSHEY_SIMPLEX, 2,
                cv::Scalar(0, 255, 255), 3);
    return frame;
}

void FakeFrameSource::close() {
    opened_ = false;
}

bool FakeFrameSource::isOpen() const {
    return opened_;
}

void FakeFrameSource::setFailureMode(FailureMode mode) {
    mode_ = mode;
}
