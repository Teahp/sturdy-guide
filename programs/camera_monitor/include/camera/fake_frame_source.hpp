#pragma once

#include "camera/frame_source.hpp"

#include <cstddef>

namespace camera {

// Deterministic test double: produces frames whose sequence number is encoded
// in a 1x1 CV_32S Mat (frame.at<int>(0, 0)). Optional failure and empty-frame
// behaviour lets tests exercise error propagation and empty-frame handling
// without any real hardware.
class FakeFrameSource final : public FrameSource {
 public:
  // When `frames_before_error` > 0, the read AFTER that many successful reads
  // throws std::runtime_error. <= 0 means never fail.
  explicit FakeFrameSource(int frames_before_error = -1);

  // Makes the read at the given 1-based call index return an empty frame
  // instead of a valid one. <= 0 disables empty frames.
  void set_empty_frame_at(int read_index);

  bool read(cv::Mat& frame) override;

  // Total number of read() calls so far (including empty frames).
  std::size_t reads() const { return reads_; }

 private:
  int frames_before_error_;
  int empty_frame_at_;
  int next_sequence_;
  std::size_t reads_;
};

}  // namespace camera
