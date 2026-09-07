#pragma once
#include "frame_source.hpp"
#include <memory>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <optional>
#include <thread>
#include <exception>

namespace sturdy_guide::camera {

// 后台采集管家：管理线程和帧缓冲
class CameraSession {
public:
    explicit CameraSession(std::unique_ptr<FrameSource> source);
    ~CameraSession();

    CameraSession(const CameraSession&) = delete;
    CameraSession& operator=(const CameraSession&) = delete;

    void start();
    void stop();
    std::optional<cv::Mat> getLatestFrame();
    void checkException();
    int width() const;
    int height() const;

private:
    void workerLoop();

    std::unique_ptr<FrameSource> source_;
    std::thread worker_;
    std::atomic<bool> running_{false};
    std::atomic<bool> stop_requested_{false};

    mutable std::mutex mutex_;
    std::condition_variable cv_;

    static constexpr size_t MAX_FRAMES = 2;
    std::vector<cv::Mat> frame_buffer_;
    size_t next_write_index_ = 0;
    bool has_new_frame_ = false;

    std::exception_ptr background_exception_;
};

} // namespace
