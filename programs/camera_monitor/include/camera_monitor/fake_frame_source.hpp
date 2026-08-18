#pragma once

#include "camera_monitor/frame_source.hpp"

#include <condition_variable>
#include <deque>
#include <exception>
#include <mutex>
#include <opencv2/core.hpp>

namespace camera_monitor {

class FakeFrameSource final : public FrameSource {
public:
    FakeFrameSource() = default;

    FakeFrameSource(const FakeFrameSource&) = delete;
    FakeFrameSource& operator=(const FakeFrameSource&) = delete;

    void push(cv::Mat frame);
    void push_empty();
    void fail_next_read();
    void close();

    cv::Mat read() override;

private:
    std::mutex mutex_;
    std::condition_variable changed_;
    std::deque<cv::Mat> frames_;
    bool fail_next_ = false;
    bool closed_ = false;
};

}  // namespace camera_monitor
