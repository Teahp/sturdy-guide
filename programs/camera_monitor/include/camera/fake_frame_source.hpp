#pragma once

#include "camera/frame_source.hpp"

#include <chrono>
#include <cstddef>
#include <opencv2/core.hpp>

#include <optional>
#include <thread>
#include <vector>

namespace sturdy_guide::camera {

// FakeFrameSource produces deterministic coloured frames from a supplied
// list. When the list is exhausted it returns std::nullopt.
// This is used for unit tests that must not depend on real hardware.
class FakeFrameSource final : public FrameSource {
 public:
  // Each element is one frame; the source returns them in order.
  // An optional read_delay simulates hardware latency between frames.
  explicit FakeFrameSource(std::vector<cv::Mat> frames,
                           std::chrono::milliseconds read_delay =
                               std::chrono::milliseconds{0});

  // Returns std::nullopt on the call after the last frame was consumed.
  [[nodiscard]] std::optional<cv::Mat> read() override;

  // Number of frames still available for read().
  [[nodiscard]] std::size_t remaining() const noexcept;

 private:
  std::vector<cv::Mat> frames_;
  std::size_t index_ = 0;
  std::chrono::milliseconds read_delay_;
};

// Creates a sequence of solid-colour test frames.
// Each frame is a unique colour derived from its index.
std::vector<cv::Mat> make_test_frames(std::size_t count, int width = 640,
                                      int height = 480);

}  // namespace sturdy_guide::camera
//继承FrameSource，模拟摄像头用于脱离硬件的测试