#include "camera/fake_frame_source.hpp"

#include <opencv2/core.hpp>

#include <stdexcept>

namespace camera {

FakeFrameSource::FakeFrameSource(int frames_before_error)
    : frames_before_error_(frames_before_error),
      empty_frame_at_(-1),
      next_sequence_(0),
      reads_(0) {}

void FakeFrameSource::set_empty_frame_at(int read_index) {
  empty_frame_at_ = read_index;
}

bool FakeFrameSource::read(cv::Mat& frame) {
  ++reads_;

  if (frames_before_error_ > 0 && reads_ > static_cast<std::size_t>(
                                         frames_before_error_)) {
    throw std::runtime_error("fake camera failure");
  }

  if (empty_frame_at_ > 0 && reads_ == static_cast<std::size_t>(empty_frame_at_)) {
    frame = cv::Mat{};  // Empty frame; must not be published.
    return true;
  }

  frame = cv::Mat(1, 1, CV_32S);
  frame.at<int>(0, 0) = ++next_sequence_;
  return true;
}

}  // namespace camera
