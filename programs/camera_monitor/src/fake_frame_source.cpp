#include "camera/fake_frame_source.hpp"

#include <stdexcept>
#include <string>
#include <thread>

namespace camera {

FakeFrameSource::FakeFrameSource() : FakeFrameSource(Options{}) {}

FakeFrameSource::FakeFrameSource(Options options) : options_(options) {}

FakeFrameSource::~FakeFrameSource() { *destroyed_flag_ = true; }

void FakeFrameSource::open() {
  ++open_calls_;
  read_calls_ = 0;  // 模拟重新打开一台设备：读取序号从头开始
  if (options_.open_throws) {
    throw std::runtime_error("fake camera failed to open");
  }
}

bool FakeFrameSource::read(cv::Mat& frame) {
  if (options_.read_delay.count() > 0) {
    std::this_thread::sleep_for(options_.read_delay);
  }
  ++read_calls_;
  const std::size_t n = read_calls_;

  if (n == options_.fail_after) {
    return false;  // 模拟设备停止返回有效帧
  }
  if (n == options_.throw_after) {
    throw std::runtime_error("fake source failed on read " +
                             std::to_string(n));
  }
  if (n == options_.empty_at) {
    frame = cv::Mat();  // 返回 true 但帧为空
    return true;
  }
  if (options_.frames != 0 && n > options_.frames) {
    return false;  // 帧数用尽，模拟设备不再返回有效帧
  }

  // 纯色帧，颜色由读取序号决定：G = n % 256，B/R 固定。
  frame = cv::Mat(options_.height, options_.width, CV_8UC3,
                  cv::Scalar(30, static_cast<double>(n % 256), 90));
  return true;
}

}  // namespace camera
