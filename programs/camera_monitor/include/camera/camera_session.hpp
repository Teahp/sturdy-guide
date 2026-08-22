#pragma once

#include "camera/frame.hpp"
#include "camera/frame_source.hpp"

#include <condition_variable>
#include <exception>
#include <memory>
#include <mutex>
#include <thread>

namespace sturdy_guide::camera {

// CameraSession 独占一个 FrameSource，管理一个后台采集线程与一个
// 有界的两帧（ping-pong）缓冲区。采集与显示通过互换槽位解耦：
// 显示落后时旧帧被覆盖丢弃，缓冲区永不增长。
//
// 线程契约：
//  - start() / stop() 由控制线程串行调用；
//  - tryTakeFrame / isRunning / hasError / rethrowError / captureFps
//    可从任意线程并发调用；
//  - stop() 幂等，可从任意线程调用（但不能是工作线程自身）。
class CameraSession {
 public:
  explicit CameraSession(std::unique_ptr<FrameSource> source);
  ~CameraSession();

  CameraSession(const CameraSession&) = delete;
  CameraSession& operator=(const CameraSession&) = delete;

  // 打开设备并启动后台线程；已运行时调用是 no-op（不会重新 open）。
  void start(int device, int width, int height);

  // 请求停止并 join；可重复调用。
  void stop();

  [[nodiscard]] bool isRunning() const;

  // 非阻塞取走最新一帧；没有新帧返回 false。绝不给空帧。
  bool tryTakeFrame(Frame& out);

  // 后台采集是否已失败。
  [[nodiscard]] bool hasError() const;

  // 若后台已失败则重抛其异常，否则直接返回。
  void rethrowError() const;

  // 采集侧统计的最近一秒帧率（非显示帧率）。
  [[nodiscard]] double captureFps() const;

 private:
  void run();
  void fail(std::exception_ptr error);

  std::unique_ptr<FrameSource> source_;

  mutable std::mutex mutex_;
  std::condition_variable frame_ready_;
  std::thread worker_;

  enum class State { kStopped, kRunning, kStopping };
  State state_ = State::kStopped;

  // 两帧 ping-pong 缓冲。worker 只写 slots_[write]（其私有下标），
  // consumer 在锁内只读 slots_[ready_]；ready_ 的翻转在锁内完成，
  // 因此像素数据的写先于发布、发布先于读取，无数据竞争。
  Frame slots_[2];
  int ready_ = -1;  // 可读槽位下标；-1 表示暂无新帧

  std::exception_ptr error_;
  double capture_fps_ = 0.0;
};

}  // namespace sturdy_guide::camera
