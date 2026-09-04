#pragma once

#include "camera/frame_source.hpp"

#include <condition_variable>
#include <cstddef>
#include <exception>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>

enum class SessionState{
    kIdle,
    kRunning,
    kFailed,
};

class CameraSession{
    public:
     explicit CameraSession(std::unique_ptr<FrameSource> source);
     ~CameraSession();

     CameraSession(const CameraSession&) = delete;
     CameraSession& operator=(const CameraSession&) = delete;

     void start();

     void stop();

     std::optional<Frame> consume_latest();
     SessionState state() const;
     std::exception_ptr error() const;

    private:
     void worker_loop();
     
     std::unique_ptr<FrameSource> source_;

     mutable std::mutex mutex_;
     std::condition_variable wake_;

     std::thread worker_;
     bool stop_requested_ = false;
     SessionState state_ = SessionState::kIdle;
     std::exception_ptr error_;

     std::optional<Frame> newest_;
     std::optional<Frame> oldest_;
};