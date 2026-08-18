#include "camera/fake_frame_source.hpp"

#include <stdexcept>

namespace sturdy_guide::camera {

FakeFrameSource::FakeFrameSource(FakeFrameSourceConfig config)
    : config_(std::move(config)) {
  if (config_.frame_size.width <= 0 || config_.frame_size.height <= 0) {
    throw std::invalid_argument("fake frame dimensions must be positive");
  }
  if (config_.frame_interval.count() < 0) {
    throw std::invalid_argument("fake frame interval cannot be negative");
  }
}

cv::Mat FakeFrameSource::read() {
  std::unique_lock lock{mutex_};
  ++read_count_;

  if (config_.frame_interval.count() > 0) {
    wake_.wait_for(lock, config_.frame_interval,
                   [this] { return stop_requested_; });//条件变量 wake_ 用于在指定的时间间隔内等待，或者在 stop_requested_ 被设置为 true 时提前唤醒。
  }
  if (stop_requested_) {
    return {};
  }

  if (empty_frames_returned_ < config_.empty_read_count) {
    ++empty_frames_returned_;
    return {};
  }
  if (config_.fail_after_successes &&
      successful_frames_ >= *config_.fail_after_successes) {
    throw std::runtime_error("simulated fake frame failure");
  }

  const auto pixel_value = static_cast<unsigned char>(successful_frames_ % 256);
  ++successful_frames_;
  const cv::Size frame_size = config_.frame_size;
  lock.unlock();
  return cv::Mat{frame_size, CV_8UC1,
                 cv::Scalar{static_cast<double>(pixel_value)}};
}

void FakeFrameSource::request_stop() noexcept {
  {
    const std::scoped_lock lock{mutex_};
    stop_requested_ = true;
  }
  wake_.notify_all();
}

std::size_t FakeFrameSource::read_count() const {
  const std::scoped_lock lock{mutex_};
  return read_count_;
}

std::size_t FakeFrameSource::empty_frames_returned() const {
  const std::scoped_lock lock{mutex_};
  return empty_frames_returned_;
}

}  // namespace sturdy_guide::camera
