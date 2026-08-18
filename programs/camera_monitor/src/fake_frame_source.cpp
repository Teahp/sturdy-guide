#include "camera/fake_frame_source.hpp"

#include <opencv2/core.hpp>

#include <stdexcept>
#include <thread>
#include <utility>

namespace camera {

FakeFrameSource::FakeFrameSource(std::vector<Step> steps)
    : steps_{std::move(steps)} {}

FakeFrameSource::FakeFrameSource(
    std::vector<Step> steps, std::shared_ptr<std::atomic_bool> destroyed_flag)
    : steps_{std::move(steps)}, destroyed_flag_{std::move(destroyed_flag)} {}

FakeFrameSource::~FakeFrameSource() {
  if (destroyed_flag_) {
    destroyed_flag_->store(true);
  }
}

FakeFrameSource::Step FakeFrameSource::frame(
    const int value, const std::chrono::milliseconds delay) {
  return Step{StepKind::Frame, value, delay, {}};
}

FakeFrameSource::Step FakeFrameSource::empty(
    const std::chrono::milliseconds delay) {
  return Step{StepKind::EmptyFrame, 0, delay, {}};
}

FakeFrameSource::Step FakeFrameSource::failure(
    std::string message, const std::chrono::milliseconds delay) {
  return Step{StepKind::Failure, 0, delay, std::move(message)};
}

void FakeFrameSource::open() {
  std::lock_guard<std::mutex> lock{mutex_};
  opened_ = true;
  next_step_ = 0;
  ++open_count_;
}

cv::Mat FakeFrameSource::read() {
  Step step;
  {
    std::lock_guard<std::mutex> lock{mutex_};
    if (!opened_) {
      throw std::runtime_error("fake frame source is not open");
    }
    if (next_step_ >= steps_.size()) {
      throw std::runtime_error("fake frame source exhausted");
    }
    step = steps_[next_step_++];
    ++read_count_;
  }

  if (step.delay.count() > 0) {
    std::this_thread::sleep_for(step.delay);
  }

  if (step.kind == StepKind::Failure) {
    throw std::runtime_error(step.message.empty() ? "fake read failure"
                                                 : step.message);
  }
  if (step.kind == StepKind::EmptyFrame) {
    return {};
  }

  cv::Mat image{32, 32, CV_8UC3,
                cv::Scalar{static_cast<double>(step.value % 255),
                           static_cast<double>((step.value * 3) % 255),
                           static_cast<double>((step.value * 7) % 255)}};
  image.at<cv::Vec3b>(0, 0) =
      cv::Vec3b{static_cast<unsigned char>(step.value % 255), 0, 0};
  return image;
}

void FakeFrameSource::close() {
  std::lock_guard<std::mutex> lock{mutex_};
  opened_ = false;
  ++close_count_;
}

std::size_t FakeFrameSource::openCount() const {
  std::lock_guard<std::mutex> lock{mutex_};
  return open_count_;
}

std::size_t FakeFrameSource::closeCount() const {
  std::lock_guard<std::mutex> lock{mutex_};
  return close_count_;
}

std::size_t FakeFrameSource::readCount() const {
  std::lock_guard<std::mutex> lock{mutex_};
  return read_count_;
}

}  // namespace camera
