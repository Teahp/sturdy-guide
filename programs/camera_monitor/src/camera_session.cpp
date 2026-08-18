#include "camera/camera_session.hpp"

#include <stdexcept>
#include <utility>

namespace camera {

CameraSession::CameraSession(std::unique_ptr<FrameSource> source)
    : source_{std::move(source)} {
  if (!source_) {
    throw std::invalid_argument("CameraSession requires a FrameSource");
  }
}

CameraSession::~CameraSession() { stop(); }

void CameraSession::start() {
  std::lock_guard<std::mutex> lifecycle_lock{lifecycle_mutex_};
  {
    std::lock_guard<std::mutex> lock{mutex_};
    if (state_ == State::Running || state_ == State::Stopping) {
      return;
    }
  }

  source_->open();

  {
    std::lock_guard<std::mutex> lock{mutex_};
    stop_requested_ = false;
    error_message_.reset();
    head_ = 0;
    count_ = 0;
    captured_frames_ = 0;
    dropped_frames_ = 0;
    state_ = State::Running;
  }

  worker_ = std::thread{&CameraSession::captureLoop, this};
}

void CameraSession::stop() {
  std::lock_guard<std::mutex> lifecycle_lock{lifecycle_mutex_};
  {
    std::lock_guard<std::mutex> lock{mutex_};
    if (state_ == State::Running) {
      state_ = State::Stopping;
    }
    stop_requested_ = true;
  }
  frame_available_.notify_all();

  if (worker_.joinable()) {
    worker_.join();
  }
  source_->close();

  {
    std::lock_guard<std::mutex> lock{mutex_};
    if (state_ != State::Idle) {
      state_ = State::Stopped;
    }
  }
  frame_available_.notify_all();
}

std::optional<CameraSession::CapturedFrame> CameraSession::tryTakeFrame() {
  std::lock_guard<std::mutex> lock{mutex_};
  return takeLatestFrameLocked();
}

std::optional<CameraSession::CapturedFrame> CameraSession::waitForFrame(
    const std::chrono::milliseconds timeout) {
  std::unique_lock<std::mutex> lock{mutex_};
  frame_available_.wait_for(lock, timeout, [&] {
    return count_ > 0 || state_ != State::Running || error_message_.has_value();
  });
  return takeLatestFrameLocked();
}

bool CameraSession::running() const {
  std::lock_guard<std::mutex> lock{mutex_};
  return state_ == State::Running && !stop_requested_;
}

CameraSession::State CameraSession::state() const {
  std::lock_guard<std::mutex> lock{mutex_};
  return state_;
}

std::optional<std::string> CameraSession::errorMessage() const {
  std::lock_guard<std::mutex> lock{mutex_};
  return error_message_;
}

std::size_t CameraSession::capturedFrames() const {
  std::lock_guard<std::mutex> lock{mutex_};
  return captured_frames_;
}

std::size_t CameraSession::droppedFrames() const {
  std::lock_guard<std::mutex> lock{mutex_};
  return dropped_frames_;
}

std::size_t CameraSession::bufferedFrames() const {
  std::lock_guard<std::mutex> lock{mutex_};
  return count_;
}

void CameraSession::captureLoop() {
  try {
    for (;;) {
      {
        std::lock_guard<std::mutex> lock{mutex_};
        if (shouldStopLocked()) {
          break;
        }
      }

      cv::Mat image = source_->read();
      if (image.empty()) {
        throw std::runtime_error("camera source returned an empty frame");
      }
      publishFrame(std::move(image));
    }
  } catch (const std::exception& error) {
    std::lock_guard<std::mutex> lock{mutex_};
    if (!stop_requested_) {
      error_message_ = error.what();
      stop_requested_ = true;
      state_ = State::Stopping;
    }
  } catch (...) {
    std::lock_guard<std::mutex> lock{mutex_};
    if (!stop_requested_) {
      error_message_ = "camera source failed with an unknown error";
      stop_requested_ = true;
      state_ = State::Stopping;
    }
  }
  frame_available_.notify_all();
}

void CameraSession::publishFrame(cv::Mat image) {
  CapturedFrame frame;
  frame.image = image.clone();
  frame.captured_at = std::chrono::steady_clock::now();

  {
    std::lock_guard<std::mutex> lock{mutex_};
    if (stop_requested_ || state_ != State::Running) {
      return;
    }

    frame.sequence = captured_frames_ + 1;
    if (count_ == kBufferCapacity) {
      head_ = (head_ + 1) % kBufferCapacity;
      --count_;
      ++dropped_frames_;
    }

    const std::size_t tail = (head_ + count_) % kBufferCapacity;
    frames_[tail] = std::move(frame);
    ++count_;
    ++captured_frames_;
  }
  frame_available_.notify_one();
}

bool CameraSession::shouldStopLocked() const {
  return stop_requested_ || state_ != State::Running;
}

std::optional<CameraSession::CapturedFrame>
CameraSession::takeLatestFrameLocked() {
  if (count_ == 0) {
    return std::nullopt;
  }

  const std::size_t latest = (head_ + count_ - 1) % kBufferCapacity;
  CapturedFrame frame = std::move(frames_[latest]);
  head_ = 0;
  count_ = 0;
  return frame;
}

}  // namespace camera
