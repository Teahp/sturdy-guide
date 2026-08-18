#pragma once

#include "camera_monitor/frame_source.hpp"

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <exception>
#include <mutex>
#include <opencv2/core.hpp>
#include <thread>

namespace camera_monitor {

class CameraSession {
public:
    explicit CameraSession(FrameSource& source);
    ~CameraSession();

    CameraSession(const CameraSession&) = delete;
    CameraSession& operator=(const CameraSession&) = delete;

    void start();
    void stop();
    void join();

    bool is_running() const;
    bool try_pop(cv::Mat& frame);
    bool wait_pop(cv::Mat& frame);
    std::size_t buffered_count() const;

private:
    void worker_loop();

    FrameSource& source_;

    mutable std::mutex mutex_;
    std::condition_variable changed_;
    std::thread worker_;
    std::deque<cv::Mat> frames_;

    bool stop_requested_ = false;
    bool running_ = false;
    std::exception_ptr error_;
};

}  // namespace camera_monitor
