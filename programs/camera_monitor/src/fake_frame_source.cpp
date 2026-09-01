# include "camera/fake_frame_source.hpp"

# include <stdexcept>
# include <utility>

FakeFrameSource::FakeFrameSource(const int width, const int height,
                                 const std::size_t frames_before_failure)
    : width_(width),
      height_(height),
      frames_before_failure_(frames_before_failure) {}

Frame FakeFrameSource::read(){
    if (frames_before_failure_ != 0 &&
        next_sequence_ >= frames_before_failure_) {
        throw std::runtime_error("fake source exhausted");
    }
    cv::Mat image(height_, width_, CV_8UC3, cv::Scalar((next_sequence_ * 4) % 256, 0, 0));
    ++next_sequence_;
    return Frame{std::move(image), next_sequence_};
}
