#pragma once

#include <string>

#include <opencv2/core.hpp>

namespace camera {

// 帧来源抽象：真实摄像头与测试假设备共用这一接口。
//
// 线程约定（由 CameraSession 保证，实现不需要自己加锁）：
//   - open() 在启动采集之前由调用线程执行一次；
//   - read() 只会被采集工作线程调用，且同一时刻最多一个线程在调用；
//   - close() 在工作线程 join 之后由调用线程执行。
//
// 数据约定：read() 必须写入调用方提供的 cv::Mat，而不是让多次调用共享同一块
// 缓冲区——CameraSession 会把帧移交给显示线程，共享缓冲区会导致画面被就地覆盖。
class FrameSource {
 public:
  virtual ~FrameSource() = default;

  FrameSource(const FrameSource&) = delete;
  FrameSource& operator=(const FrameSource&) = delete;
  FrameSource(FrameSource&&) = delete;
  FrameSource& operator=(FrameSource&&) = delete;

  // 打开设备。失败返回 false，不抛异常。
  [[nodiscard]] virtual bool open() = 0;

  // 释放设备资源。必须可以重复调用，并且不允许抛异常。
  virtual void close() noexcept = 0;

  [[nodiscard]] virtual bool is_open() const noexcept = 0;

  // 阻塞读取一帧。
  //   - 返回 true 且 frame 非空：成功；
  //   - 返回 true 但 frame 为空：设备给了空帧，由 CameraSession 丢弃，绝不发布；
  //   - 返回 false：设备已结束或不可用，CameraSession 视为采集失败并上报；
  //   - 抛异常：不可恢复错误，由 CameraSession 捕获后转交调用线程。
  virtual bool read(cv::Mat& frame) = 0;

  // 人类可读的设备描述，用于窗口标题和错误信息。
  [[nodiscard]] virtual std::string description() const = 0;

 protected:
  FrameSource() = default;
};

}  // namespace camera
