#include "camera/camera_session.hpp"

#include <stdexcept>
#include <utility>

namespace camera {

CameraSession::CameraSession(std::unique_ptr<FrameSource> source)
    : source_(std::move(source)) {}

CameraSession::~CameraSession() { stop(); }

void CameraSession::start() {
  std::unique_lock<std::mutex> lock(mutex_);
  if (started_) {
    return;  // Contract: repeated start() is a no-op.
  }
  started_ = true;
  stop_requested_ = false;
  frames_.clear();
  error_ = nullptr;

  try {
    // Create the thread while holding the lock: if construction throws (e.g.
    // resource exhaustion) we roll the state back so a later start() can
    // retry, and stop() can never observe a half-started session.
    worker_ = std::thread(&CameraSession::worker_loop, this);
  } catch (...) {
    started_ = false;
    throw;
  }
}

void CameraSession::stop() {
  std::thread worker_to_join;
  {
    std::unique_lock<std::mutex> lock(mutex_);
    if (!started_) {
      return;  // Contract: stop() before start() or after stop() is a no-op.
    }
    stop_requested_ = true;
    cv_.notify_all();
    worker_to_join = std::move(worker_);
  }

  // Join outside the lock: the worker takes the mutex while finishing.
  if (worker_to_join.joinable()) {
    worker_to_join.join();
  }

  {
    std::unique_lock<std::mutex> lock(mutex_);
    started_ = false;
  }
}

bool CameraSession::running() const {
  std::unique_lock<std::mutex> lock(mutex_);
  return started_;
}

CameraSession::FrameStatus CameraSession::wait_for_frame(
    cv::Mat& frame, std::chrono::milliseconds timeout) {
  std::unique_lock<std::mutex> lock(mutex_);
  cv_.wait_for(lock, timeout, [this] {
    return !frames_.empty() || stop_requested_ || error_ != nullptr;
  });

  if (!frames_.empty()) {
    // Return the newest frame. Older buffered frames stay queued and are
    // returned by later calls when the consumer outpaces the producer; the
    // worker keeps the queue bounded (kMaxBufferedFrames) by dropping the
    // oldest frames on the way in.
    frame = std::move(frames_.back());
    frames_.pop_back();
    return FrameStatus::NewFrame;
  }

  if (stop_requested_ || error_ != nullptr) {
    return FrameStatus::Ended;
  }
  return FrameStatus::TimedOut;
}

bool CameraSession::has_error() const {
  std::unique_lock<std::mutex> lock(mutex_);
  return error_ != nullptr;
}

std::exception_ptr CameraSession::error() const {
  std::unique_lock<std::mutex> lock(mutex_);
  return error_;
}

void CameraSession::worker_loop() {
  for (;;) {
    {
      std::unique_lock<std::mutex> lock(mutex_);
      if (stop_requested_) {
        return;
      }
    }

    // Slow I/O happens OUTSIDE the critical section.
    cv::Mat frame;
    bool ok = false;
    try {
      ok = source_->read(frame);
    } catch (...) {
      // Capture any exception (not only std::exception) so a misbehaving
      // FrameSource can never escape the worker thread and std::terminate.
      std::unique_lock<std::mutex> lock(mutex_);
      error_ = std::current_exception();
      cv_.notify_all();
      return;
    }

    if (!ok) {
      std::unique_lock<std::mutex> lock(mutex_);
      error_ = std::make_exception_ptr(
          std::runtime_error("camera stopped returning valid frames"));
      cv_.notify_all();
      return;
    }

    if (frame.empty()) {
      continue;  // Empty frames are never published.
    }

    {
      std::unique_lock<std::mutex> lock(mutex_);
      frames_.push_back(std::move(frame));
      while (frames_.size() > kMaxBufferedFrames) {
        frames_.pop_front();  // Drop the oldest frame to bound the buffer.
      }
      cv_.notify_all();
    }
  }
}

}  // namespace camera
