#include "camera/fake_frame_source.hpp"

namespace sturdy_guide::camera {

FakeFrameSource::FakeFrameSource(std::vector<cv::Mat> frames,
                                 std::chrono::milliseconds read_delay)
    : frames_(std::move(frames)), read_delay_(read_delay) {}

std::optional<cv::Mat> FakeFrameSource::read() {
  if (read_delay_.count() > 0) {
    std::this_thread::sleep_for(read_delay_);
  }
  if (index_ >= frames_.size()) {
    return std::nullopt;
  }
  return frames_[index_++];
}

std::size_t FakeFrameSource::remaining() const noexcept {
  if (index_ >= frames_.size()) {
    return 0;
  }
  return frames_.size() - index_;
}

std::vector<cv::Mat> make_test_frames(const std::size_t count, const int width,
                                      const int height) {
  std::vector<cv::Mat> frames;
  frames.reserve(count);
  for (std::size_t i = 0; i < count; ++i) {
    cv::Mat frame(height, width, CV_8UC3);
    // Each frame gets a unique solid colour derived from its index.
    const auto hue = static_cast<int>(i * 37 % 180);
    frame.setTo(cv::Scalar(hue, 128, 200));
    frames.push_back(frame);
  }
  return frames;
}

}  // namespace sturdy_guide::camera
