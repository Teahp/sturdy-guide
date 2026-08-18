#pragma once
#include <memory>
#include <optional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <exception>
#include "frame_source.h"

class CameraSession {
public:
    explicit CameraSession(std::unique_ptr<FrameSource> source);
    ~CameraSession();

    void start();
    void stop();
    void join();
    bool isRunning() const;

    std::optional<cv::Mat> getLatestFrame();
    std::exception_ptr getException() const;

private:
    void workerLoop();

    std::unique_ptr<FrameSource> source_;
    std::thread worker_;
    mutable std::mutex mtx_;
    std::condition_variable cv_;
    std::queue<cv::Mat> buffer_;
    static constexpr size_t MAX_BUFFER = 2;
    bool running_ = false;
    bool stop_requested_ = false;
    std::exception_ptr last_exception_;
};
