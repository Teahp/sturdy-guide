#include "camera/fake_frame_source.h"

#include <opencv2/imgproc.hpp>

#include <sstream>
#include <stdexcept>
#include <thread>

namespace camera {

FakeFrameSource::FakeFrameSource(Options options) : options_(options) {}

bool FakeFrameSource::open() {
  released_.store(false);
  return true;
}

bool FakeFrameSource::read(cv::Mat& frame) {
  const std::size_t read_index = reads_.fetch_add(1);

  // 模拟前几帧为空帧：read 返回 true 但 frame 无效。
  if (read_index < options_.leading_empty_frames) {
    frame.release();
    return true;
  }

  std::this_thread::sleep_for(options_.frame_interval);

  // 模拟读取失败：产生指定帧数后抛异常，由 CameraSession 捕获并回传。
  if (options_.fail_after_frames > 0 &&
      produced_.load() >= options_.fail_after_frames) {
    throw std::runtime_error("fake camera read failure (simulated)");
  }

  const std::size_t number = produced_.fetch_add(1) + 1;
  // 每一帧颜色随序号变化，保证帧内容可区分、可验证。
  frame = cv::Mat(options_.height, options_.width, CV_8UC3,
                  cv::Scalar(20, 40 + static_cast<int>(number % 200), 80));
  std::ostringstream label;
  label << "fake frame " << number;
  cv::putText(frame, label.str(), cv::Point{16, 32},
              cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar{255, 255, 255}, 1,
              cv::LINE_AA);
  return true;
}

void FakeFrameSource::release() { released_.store(true); }

std::size_t FakeFrameSource::frames_produced() const {
  return produced_.load();
}

bool FakeFrameSource::released() const { return released_.load(); }

}  // namespace camera
