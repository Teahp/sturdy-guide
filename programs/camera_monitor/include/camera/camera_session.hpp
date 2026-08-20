#pragma once

#include "camera/frame_source.hpp"

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <exception>
#include <memory>
#include <mutex>
#include <thread>

namespace camera {

// Owns exactly one FrameSource, runs it on a dedicated worker thread, keeps a
// bounded buffer of the most recent frames, and forwards worker exceptions
// back to the calling thread.
//
// Thread-safety contract (see README.md for the full Q&A):
//  - start() and stop() may be called from any thread.
//  - start() when already started is a no-op.
//  - stop() when not started or already stopped is a safe no-op.
//  - wait_for_frame() may be called from multiple consumer threads.
//  - The worker never calls read(), imshow() or imwrite() while holding the
//    internal mutex; slow I/O stays outside the critical section.
class CameraSession {
 public:
  // The buffer never holds more than this many frames; when full, the oldest
  // frame is dropped to keep latency bounded.
  static constexpr std::size_t kMaxBufferedFrames = 2;

  explicit CameraSession(std::unique_ptr<FrameSource> source);

  // Stops the worker thread and joins it (same as stop()).
  ~CameraSession();

  CameraSession(const CameraSession&) = delete;
  CameraSession& operator=(const CameraSession&) = delete;

  // Starts the background capture thread. No-op if already started.
  void start();

  // Requests the worker to stop, wakes it up, and joins the thread.
  // Safe to call repeatedly; safe to call before start().
  void stop();

  bool running() const;

  // Result of waiting for a frame.
  enum class FrameStatus {
    NewFrame,  // `frame` holds a valid, non-empty frame.
    TimedOut,  // No frame arrived within `timeout`; session is still running.
    Ended,     // Session has stopped (or failed) and the buffer is drained.
  };

  // Blocks until a frame is available, the timeout elapses, or the session
  // ends. `frame` is only valid when the result is NewFrame.
  FrameStatus wait_for_frame(cv::Mat& frame,
                             std::chrono::milliseconds timeout);

  // True when the worker thread reported an error (read failure or exception).
  bool has_error() const;

  // The worker's exception, or nullptr. Call only after has_error().
  std::exception_ptr error() const;

 private:
  void worker_loop();

  std::unique_ptr<FrameSource> source_;

  mutable std::mutex mutex_;
  std::condition_variable cv_;
  std::thread worker_;

  bool started_ = false;
  bool stop_requested_ = false;
  std::deque<cv::Mat> frames_;
  std::exception_ptr error_;
};

}  // namespace camera
