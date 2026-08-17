#include "camera/fake_frame_source.hpp"

#include <opencv2/core.hpp>
#include <stdexcept>
#include <utility>

namespace camera {

FakeFrameSource::FakeFrameSource()
    : FakeFrameSource(Options{}) {}

FakeFrameSource::FakeFrameSource(Options options)
    : options_(std::move(options)) {}

bool FakeFrameSource::open() {
  opened_ = true;
  return true;
}

bool FakeFrameSource::read(cv::Mat& frame) {
  if (!opened_) {
    return false;
  }

  if (options_.frames_before_failure > 0 &&
      frames_read_ >= options_.frames_before_failure) {
    switch (options_.failure_mode) {
      case FailureMode::ReturnFalse:
        return false;
      case FailureMode::Throw:
        throw std::runtime_error("fake camera read failure");
      case FailureMode::Empty:
        frame = cv::Mat();
        return true;
      case FailureMode::None:
        break;
    }
  }

  // 把序号编码进像素：B=低 8 位，G=中 8 位，R=高 8 位。
  const std::size_t index = frames_read_++;
  frame = cv::Mat(options_.height, options_.width, CV_8UC3,
                  cv::Scalar(index & 0xFF, (index >> 8) & 0xFF,
                             (index >> 16) & 0xFF));
  return true;
}

void FakeFrameSource::close() {
  opened_ = false;
}

}  // namespace camera
