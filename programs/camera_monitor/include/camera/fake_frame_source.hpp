#pragma once

#include "camera/frame_source.hpp"

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <optional>

namespace sturdy_guide::camera {
//用于测试
struct FakeFrameSourceConfig {
  cv::Size frame_size{8, 6};//默认帧大小为 8x6 像素。
  std::chrono::milliseconds frame_interval{1};//默认帧间隔为 1 毫秒。
  std::size_t empty_read_count = 0;//默认空读取次数为 0。
  std::optional<std::size_t> fail_after_successes;//可选的失败条件，表示在成功读取指定次数后失败。
};

// A deterministic source for tests. Every successful read produces a new
// CV_8UC1 image whose pixels contain the source frame number modulo 256.
class FakeFrameSource final : public FrameSource {
 public:
  explicit FakeFrameSource(FakeFrameSourceConfig config = {});

  cv::Mat read() override;
  void request_stop() noexcept override;

  [[nodiscard]] std::size_t read_count() const;
  [[nodiscard]] std::size_t empty_frames_returned() const;

 private:
  FakeFrameSourceConfig config_;
  mutable std::mutex mutex_;
  std::condition_variable wake_;
  bool stop_requested_ = false;
  std::size_t read_count_ = 0;
  std::size_t empty_frames_returned_ = 0;
  std::size_t successful_frames_ = 0;
};

}  // namespace sturdy_guide::camera
