#pragma once

#include "FrameSource.hpp"
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>
#include <thread>

#include <opencv2/core.hpp>

class CameraSession {
public:
    explicit CameraSession(std::unique_ptr<FrameSource> source);
    ~CameraSession();

    void start();
    void stop();
    void join();

    std::optional<cv::Mat> getLatestFrame();

private:
    void acquisitionLoop();

    std::unique_ptr<FrameSource> source_;
    std::atomic<bool> running_{false};
    std::thread worker_;

    std::mutex mutex_;
    std::queue<cv::Mat> frameQueue_;
    static constexpr size_t MAX_QUEUE_SIZE = 2;
};
