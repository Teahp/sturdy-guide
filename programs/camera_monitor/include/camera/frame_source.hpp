#pragma once

#include <opencv2/core.hpp>

namespace camera {

// Abstract frame source. Both the real camera and test doubles implement
// this interface so that production code never needs a "test mode" branch.
class FrameSource {
 public:
  virtual ~FrameSource() = default;

  // Reads one frame into `frame`. Returns true on success, false when the
  // stream ends or fails. A true return with an empty `frame` means an empty
  // frame, which callers must not publish as a valid frame.
  virtual bool read(cv::Mat& frame) = 0;
};

}  // namespace camera
