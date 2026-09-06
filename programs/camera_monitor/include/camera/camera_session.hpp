#pragma once
#include "camera/frame.hpp"
#include "camera/frame_source.hpp"
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <exception>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>

namespace sturdy_guide::camera
 {

enum class SessionState 
{
    stopped,
    running,
    stopping,
    failed
};

class CameraSession 
{
public:
    explicit CameraSession(std::unique_ptr<FrameSource> source);
    ~CameraSession();

    CameraSession(const CameraSession&) = delete;
    CameraSession& operator=(const CameraSession&) = delete;

    void start();
    void stop() noexcept;

    std::optional<Frame> latest_frame();
    SessionState state() const;
    std::size_t buffered_frame_count() const;
    std::size_t dropped_frame_count() const;
    void rethrow_background_error() const;

private:
    void worker_loop();

    std::unique_ptr<FrameSource> source_;
    mutable std::mutex mutex_;
    std::condition_variable condition_;
    std::deque<Frame> frames_;
    std::thread worker_;
    SessionState state_{SessionState::stopped};
    std::exception_ptr background_error_;
    std::size_t next_sequence_{0};
    std::size_t dropped_frames_{0};
};

}