#pragma once

#include "camera/frame_source.hpp"

#include <array>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

namespace camera {

class CameraSession {
 public:
  struct CapturedFrame {
    std::size_t sequence{0};
    cv::Mat image;
    std::chrono::steady_clock::time_point captured_at{};
  };

  enum class State {
    Idle,
    Running,
    Stopping,
    Stopped,
  };

  explicit CameraSession(std::unique_ptr<FrameSource> source);
  CameraSession(const CameraSession&) = delete;
  CameraSession& operator=(const CameraSession&) = delete;
  ~CameraSession();

  void start();
  void stop();

  std::optional<CapturedFrame> tryTakeFrame();
  std::optional<CapturedFrame> waitForFrame(
      std::chrono::milliseconds timeout);

  bool running() const;
  State state() const;
  std::optional<std::string> errorMessage() const;
  std::size_t capturedFrames() const;
  std::size_t droppedFrames() const;
  std::size_t bufferedFrames() const;

 private:
  void captureLoop();
  void publishFrame(cv::Mat image);
  bool shouldStopLocked() const;
  std::optional<CapturedFrame> takeLatestFrameLocked();

  static constexpr std::size_t kBufferCapacity = 2;

  std::unique_ptr<FrameSource> source_;
  mutable std::mutex mutex_;
  std::condition_variable frame_available_;
  std::mutex lifecycle_mutex_;
  std::thread worker_;
  State state_{State::Idle};
  bool stop_requested_{false};
  std::optional<std::string> error_message_;
  std::array<CapturedFrame, kBufferCapacity> frames_;
  std::size_t head_{0};
  std::size_t count_{0};
  std::size_t captured_frames_{0};
  std::size_t dropped_frames_{0};
};

}  // namespace camera
