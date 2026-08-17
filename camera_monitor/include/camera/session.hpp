#pragma once

#include "frame_source.hpp"
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <optional>
#include <vector>
#include <exception>

namespace camera {

class CameraSession {
public:
    explicit CameraSession(std::unique_ptr<FrameSource> source);
    ~CameraSession();

    CameraSession(const CameraSession&) = delete;
    CameraSession& operator=(const CameraSession&) = delete;

    void start();
    void stop();
    bool isRunning() const;

    bool getLatestFrame(Frame& outFrame);
    bool hasException() const;   // 新增：检查是否有后台异常
    void rethrowException();     // 如果有异常则抛出，否则无操作

private:
    void workerLoop();

    std::unique_ptr<FrameSource> source_;
    std::thread worker_;
    std::atomic<bool> running_{false};
    std::atomic<bool> shouldStop_{false};

    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::vector<Frame> buffer_;
    bool bufferFull_ = false;

    std::exception_ptr exceptionPtr_ = nullptr;
    mutable std::mutex exceptionMutex_;
};

} // namespace camera
