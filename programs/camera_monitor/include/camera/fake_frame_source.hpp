#pragma once

#include "camera/frame_source.hpp"

#include <chrono>
#include <cstddef>
#include <limits>
#include <memory>

namespace camera {

// 测试替身：不访问任何真实设备，按选项产生确定的帧、空帧或失败。
// 帧内容是纯色，颜色由读取序号决定，便于断言“拿到的是哪一帧”。
class FakeFrameSource final : public FrameSource {
 public:
  static constexpr std::size_t kNever =
      std::numeric_limits<std::size_t>::max();

  struct Options {
    int width = 320;
    int height = 240;

    // 正常帧数量上限；0 表示无限产生。
    std::size_t frames = 0;

    // 第 fail_after 次 read() 返回 false（设备停止返回有效帧）。
    std::size_t fail_after = kNever;

    // 第 throw_after 次 read() 抛出 std::runtime_error。
    std::size_t throw_after = kNever;

    // 第 empty_at 次 read() 返回一个空 Mat（但返回 true）。
    std::size_t empty_at = kNever;

    // open() 是否抛出，用于测试打开失败路径。
    bool open_throws = false;

    // 每次 read() 的模拟采集耗时；0 表示立即返回。
    std::chrono::milliseconds read_delay{};
  };

  // GCC 11 对“嵌套聚合类型的 = {} 默认实参”有解析缺陷，
  // 因此改用委托构造：默认构造在 .cpp 中把 Options{} 传给显式构造。
  FakeFrameSource();
  explicit FakeFrameSource(Options options);
  ~FakeFrameSource() override;

  void open() override;
  bool read(cv::Mat& frame) override;
  int width() const override { return options_.width; }
  int height() const override { return options_.height; }

  std::size_t open_calls() const { return open_calls_; }
  std::size_t read_calls() const { return read_calls_; }

  // 析构顺序测试：共享标志在析构函数中置位，测试持有 shared_ptr 副本，
  // 因此销毁会话之后读取该标志是安全的。
  std::shared_ptr<bool> destroyedFlag() const { return destroyed_flag_; }

 private:
  Options options_;
  std::size_t open_calls_ = 0;
  std::size_t read_calls_ = 0;
  std::shared_ptr<bool> destroyed_flag_ = std::make_shared<bool>(false);
};

}  // namespace camera
