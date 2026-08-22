#include "fake_frame_source.hpp"

#include <opencv2/core.hpp>

#include <stdexcept>
#include <thread>

namespace sturdy_guide::camera {

FakeFrameSource::FakeFrameSource(Options options) : options_(options) {}

bool FakeFrameSource::open(int device, int width, int height) {
  static_cast<void>(device);
  static_cast<void>(width);
  static_cast<void>(height);
  ++open_calls_;
  return true;
}

bool FakeFrameSource::read(Frame& out) {
  ++reads_in_flight_;
  struct OnExit {
    std::atomic<int>& counter;
    ~OnExit() { --counter; }
  } decrement{reads_in_flight_};

  if (options_.delay.count() > 0) {
    std::this_thread::sleep_for(options_.delay);
  }

  const std::uint64_t sequence = produced_.fetch_add(1);

  if (options_.throw_after >= 0 &&
      sequence >= static_cast<std::uint64_t>(options_.throw_after)) {
    throw std::runtime_error("simulated capture failure");
  }
  if (options_.fail_after >= 0 &&
      sequence >= static_cast<std::uint64_t>(options_.fail_after)) {
    last_error_ = "simulated end of stream";
    return false;
  }

  out.sequence = sequence;
  if (options_.empty_every > 0 &&
      sequence % static_cast<std::uint64_t>(options_.empty_every) ==
          std::uint64_t{0}) {
    out.image = cv::Mat();  // 空帧：由会话层跳过，不发布
    return true;
  }

  out.image = cv::Mat(options_.height, options_.width, CV_8UC3,
                      cv::Scalar(static_cast<int>(sequence & 0xFF),
                                 static_cast<int>((sequence >> 8) & 0xFF),
                                 static_cast<int>((sequence >> 16) & 0xFF)));
  return true;
}

std::string FakeFrameSource::lastError() const { return last_error_; }

}  // namespace sturdy_guide::camera
