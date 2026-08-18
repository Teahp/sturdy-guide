#include "camera_monitor/camera_session.hpp"

#include <stdexcept>
#include <utility>

namespace camera_monitor {

CameraSession::CameraSession(FrameSource& source) : source_(source) {}

CameraSession::~CameraSession() {
    stop();
    try {
        join();
    } catch (...) {
    }
}

void CameraSession::start() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (running_) {
        return;
    }
    if (worker_.joinable()) {
        throw std::logic_error("join previous camera session before restarting");
    }

    stop_requested_ = false;
    running_ = true;
    error_ = nullptr;
    frames_.clear();
    worker_ = std::thread(&CameraSession::worker_loop, this);
}

void CameraSession::stop() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stop_requested_ = true;
    }
    changed_.notify_all();
}

void CameraSession::join() {
    if (worker_.joinable()) {
        worker_.join();
    }

    std::exception_ptr error;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        error = error_;
    }

    if (error) {
        std::rethrow_exception(error);
    }
}

bool CameraSession::is_running() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return running_;
}

bool CameraSession::try_pop(cv::Mat& frame) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (frames_.empty()) {
        return false;
    }

    frame = std::move(frames_.back());
    frames_.clear();
    return true;
}

bool CameraSession::wait_pop(cv::Mat& frame) {
    std::unique_lock<std::mutex> lock(mutex_);
    changed_.wait(lock, [this] {
        return stop_requested_ || !frames_.empty() || error_ != nullptr || !running_;
    });

    if (frames_.empty()) {
        return false;
    }

    frame = std::move(frames_.back());
    frames_.clear();
    return true;
}

std::size_t CameraSession::buffered_count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return frames_.size();
}

void CameraSession::worker_loop() {
    try {
        for (;;) {
            {
                std::lock_guard<std::mutex> lock(mutex_);
                if (stop_requested_) {
                    break;
                }
            }

            cv::Mat frame = source_.read();
            if (frame.empty()) {
                continue;
            }

            {
                std::lock_guard<std::mutex> lock(mutex_);
                if (stop_requested_) {
                    break;
                }

                frames_.push_back(std::move(frame));
                while (frames_.size() > 2) {
                    frames_.pop_front();
                }
            }
            changed_.notify_all();
        }
    } catch (...) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            error_ = std::current_exception();
        }
        changed_.notify_all();
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        running_ = false;
    }
    changed_.notify_all();
}

}  // namespace camera_monitor
