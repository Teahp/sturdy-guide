#include "camera_monitor/fake_frame_source.hpp"

#include <chrono>
#include <stdexcept>

namespace camera_monitor {

void FakeFrameSource::push(cv::Mat frame) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        frames_.push_back(std::move(frame));
    }
    changed_.notify_all();
}

void FakeFrameSource::push_empty() {
    push(cv::Mat{});
}

void FakeFrameSource::fail_next_read() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        fail_next_ = true;
    }
    changed_.notify_all();
}

void FakeFrameSource::close() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        closed_ = true;
    }
    changed_.notify_all();
}

cv::Mat FakeFrameSource::read() {
    std::unique_lock<std::mutex> lock(mutex_);
    changed_.wait_for(lock, std::chrono::milliseconds(5), [this] {
        return fail_next_ || closed_ || !frames_.empty();
    });

    if (fail_next_) {
        fail_next_ = false;
        throw std::runtime_error("fake frame source read failure");
    }

    if (frames_.empty()) {
        return {};
    }

    cv::Mat frame = std::move(frames_.front());
    frames_.pop_front();
    return frame;
}

}  // namespace camera_monitor
