#include "camera/camera_session.hpp"

#include <stdexcept>

namespace sturdy_guide::camera {

CameraSession::CameraSession(std::unique_ptr<FrameSource> source)
    : source_(std::move(source)) {}

CameraSession::~CameraSession() {
  stop();
  try {
    join();
  } catch (...) {
  }
}

void CameraSession::start() {
  {
    std::scoped_lock lock{mutex_};
    if (running_) {
      throw std::logic_error("camera session is already running");
    }
    stop_requested_ = false;
    worker_error_ = nullptr;
    buffer_[0].reset();
    buffer_[1].reset();
    buffer_dirty_[0] = false;
    buffer_dirty_[1] = false;
    running_ = true;
  }
  worker_ = std::thread(&CameraSession::run, this);
}

void CameraSession::stop() {
  {
    const std::scoped_lock lock{mutex_};
    stop_requested_ = true;
  }
}

void CameraSession::join() {
  std::thread worker;
  {
    std::scoped_lock lock{mutex_};
    if (worker_.joinable()) {
      worker = std::move(worker_);
    }
  }
  if (worker.joinable()) {
    worker.join();
  }
}

bool CameraSession::is_running() const {
  const std::scoped_lock lock{mutex_};
  return running_;
}

std::optional<cv::Mat> CameraSession::latest_frame() {
  const std::scoped_lock lock{mutex_};
  // Return the newest frame that has been consumed.
  if (buffer_dirty_[1]) {
    return buffer_[1];
  }
  if (buffer_dirty_[0]) {
    return buffer_[0];
  }
  return std::nullopt;
}

std::exception_ptr CameraSession::error() const {
  const std::scoped_lock lock{mutex_};
  return worker_error_;
}

void CameraSession::run() {
  try {
    while (true) {
      {
        const std::scoped_lock lock{mutex_};
        if (stop_requested_) {
          break;
        }
      }

      auto frame = source_->read();
      if (!frame.has_value()) {
        throw std::runtime_error("camera stopped returning valid frames");
      }

      const std::scoped_lock lock{mutex_};
      if (stop_requested_) {
        break;
      }
      // Buffer at most two frames: discard the older one when a new
      // frame arrives, always keeping the newest.
      if (buffer_dirty_[1]) {
        // Slot 1 already holds a frame; shift it to slot 0 and
        // overwrite slot 1 with the newest frame.
        buffer_[0] = std::move(buffer_[1]);
        buffer_dirty_[0] = true;
      }
      buffer_[1] = std::move(frame);
      buffer_dirty_[1] = true;
    }
  } catch (...) {
    const std::scoped_lock lock{mutex_};
    worker_error_ = std::current_exception();
  }

  {
    const std::scoped_lock lock{mutex_};
    running_ = false;
  }
}

}  // namespace sturdy_guide::camera
