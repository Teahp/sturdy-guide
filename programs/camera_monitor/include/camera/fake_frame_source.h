// 测试用假帧源：产生确定性的合成帧，可模拟慢速、读取失败与空帧。
// 所有配置在构造时固定，运行期间只读。
#pragma once

#include "camera/frame_source.h"

#include <atomic>
#include <chrono>
#include <cstddef>

namespace camera {

// 配置参数独立为命名空间作用域类型（而非嵌套类）。
// 原因：GCC 不允许在封闭类构造函数默认实参中使用嵌套聚合类型的默认成员
// 初始化器（报错 "default member initializer ... required before the end
// of its enclosing class"），放到命名空间作用域可规避该限制。
struct FakeFrameSourceOptions {
  int width = 640;
  int height = 480;
  std::chrono::milliseconds frame_interval{33};  // 每帧间隔，约 30 FPS
  std::size_t fail_after_frames = 0;   // >0：产生这么多帧后 read() 抛异常
  std::size_t leading_empty_frames = 0;  // 前 N 次 read() 返回空帧
};

class FakeFrameSource : public FrameSource {
 public:
  // 保留 Options 别名，调用方仍可写 FakeFrameSource::Options{...}。
  using Options = FakeFrameSourceOptions;

  explicit FakeFrameSource(Options options = {});

  bool open() override;
  bool read(cv::Mat& frame) override;
  void release() override;

  // 已成功产生的帧数（线程安全，测试用于断言递增序号）。
  std::size_t frames_produced() const;
  // 是否已被 release()（线程安全，测试用于断言停止/析构路径）。
  bool released() const;

 private:
  Options options_;
  std::atomic<std::size_t> reads_{0};
  std::atomic<std::size_t> produced_{0};
  std::atomic<bool> released_{false};
};

}  // namespace camera
