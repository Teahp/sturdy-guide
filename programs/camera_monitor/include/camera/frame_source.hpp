#pragma once

#include <opencv2/core.hpp>

#include <optional>

namespace sturdy_guide::camera {

// FrameSource is the abstract interface for all frame providers.
// Concrete implementations include OpenCvCamera (real device) and
// FakeFrameSource (deterministic mock for CI).
class FrameSource {
 public:
  virtual ~FrameSource() = default;

  // Returns the next captured frame, or std::nullopt on failure.
  // A failure may be transient (e.g. device busy) or permanent
  // (e.g. device disconnected). Callers must not call read() again
  // after receiving std::nullopt.
  [[nodiscard]] virtual std::optional<cv::Mat> read() = 0;
};

}  // namespace sturdy_guide::camera
//统一的图像读取接口，实现opencvcamera与fakeframesource的隔离

