#pragma once

#include <opencv2/core.hpp>

namespace camera {

class FrameSource {
 public:
  virtual ~FrameSource() = default;

  virtual void open() = 0;
  virtual cv::Mat read() = 0;
  virtual void close() = 0;
};

}  // namespace camera
