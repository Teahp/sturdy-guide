#ifndef CAMERA_FAKE_FRAME_SOURCE_HPP
#define CAMERA_FAKE_FRAME_SOURCE_HPP

#include "camera/frame_source.hpp"

#include <cstddef>

namespace camera {

// 测试用假设备：不访问任何硬件，按序号生成确定性的纯色帧。
// 每个像素的 B/G/R 通道分别保存序号的低/中/高 8 位，测试可解码校验序号。
// 可配置在若干帧之后模拟：返回失败、抛出异常或产生空帧。
class FakeFrameSource final : public FrameSource {
 public:
  enum class FailureMode {
    None,         // 永不失败
    ReturnFalse,  // read() 返回 false
    Throw,        // read() 抛出异常
    Empty,        // read() 返回 true 但 frame 为空
  };

  struct Options {
    int width = 320;
    int height = 240;
    std::size_t frames_before_failure = 0;  // 0 表示不触发失败
    FailureMode failure_mode = FailureMode::None;
  };

  FakeFrameSource();
  explicit FakeFrameSource(Options options);

  bool open() override;
  bool read(cv::Mat& frame) override;
  void close() override;

 private:
  Options options_;
  bool opened_ = false;
  std::size_t frames_read_ = 0;
};

}  // namespace camera

#endif  // CAMERA_FAKE_FRAME_SOURCE_HPP
