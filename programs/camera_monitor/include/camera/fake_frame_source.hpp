#pragma once

#include "camera/frame_source.hpp"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <limits>
#include <memory>
#include <string>

namespace camera {

// 测试观察点：让测试能验证采集线程真的被 join、设备真的被关闭，
// 而不需要接触摄像头，也不需要断言"程序没崩"这种弱证据。
struct SourceProbe {
  std::atomic<int> open_calls{0};
  std::atomic<int> close_calls{0};
  std::atomic<int> reads{0};
  std::atomic<int> reads_in_flight{0};
  std::atomic<bool> destroyed{false};
};

// 产生确定性假帧的 FrameSource，用于不依赖真实硬件的测试。
//
// 配置放在命名空间作用域而不是嵌套在类里：嵌套类的默认成员初始化器要等外层类定义结束
// 才可用，而构造函数的默认实参 `{}` 在外层类内部就要被检查，两者冲突。
// 类内保留 `using Config = FakeFrameSourceConfig;` 所以调用方看到的还是 FakeFrameSource::Config。
struct FakeFrameSourceConfig {
  // 0 表示无限产帧；否则第 total_frames 帧之后开始返回 false。
  std::size_t total_frames = 0;
  // 每次读取前的人为延时，用来模拟设备节奏。
  std::chrono::microseconds period{0};
  // 第 N 次读取返回空帧（自 1 开始计），kNever 表示不模拟。
  std::size_t empty_at = std::numeric_limits<std::size_t>::max();
  // 第 N 次读取抛出异常（自 1 开始计），kNever 表示不模拟。
  std::size_t throw_at = std::numeric_limits<std::size_t>::max();
};

class FakeFrameSource final : public FrameSource {
 public:
  static constexpr std::size_t kNever = std::numeric_limits<std::size_t>::max();
  using Config = FakeFrameSourceConfig;

  explicit FakeFrameSource(Config config = {},
                           std::shared_ptr<SourceProbe> probe = nullptr);
  ~FakeFrameSource() override;

  [[nodiscard]] bool open() override;
  void close() noexcept override;
  [[nodiscard]] bool is_open() const noexcept override;
  bool read(cv::Mat& frame) override;
  [[nodiscard]] std::string description() const override;

 private:
  Config config_;
  std::shared_ptr<SourceProbe> probe_;
  bool open_ = false;
  std::size_t reads_ = 0;
};

}  // namespace camera
