#pragma once

#include "camera/frame.hpp"
#include "camera/frame_source.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <exception>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

namespace camera {

// 独占一个 FrameSource，管理「采集线程 + 有界最新帧缓冲 + 停止状态机」。
//
// 线程模型：
//   - 工作线程：只调用 FrameSource::read()，把非空帧发布进有界缓冲；
//   - 调用线程：调用 start() / stop() / wait_for_frame() / stats()。
//
// 状态机：Idle --start()--> Running --stop()--> Idle，可在 Idle 与 Running 之间反复。
// start() 在 Running 状态下抛 std::logic_error；stop() 在任意状态都可重复调用。
//
// 锁的边界：mutex_ 只保护 buffer_ / stats_ / error_ / stop_requested_ 的完整转换。
// read() 在工作线程、且不在临界区内执行；imshow() / imwrite() 根本不在本类中。
class CameraSession {
 public:
  // 缓冲区容量：只保留最新的两帧，溢出时丢弃最旧的一帧。
  static constexpr std::size_t kMaxBufferedFrames = 2;

  struct Stats {
    std::uint64_t captured = 0;   // 已发布的采集帧数
    std::uint64_t delivered = 0;  // 已被调用线程取走的帧数
    std::uint64_t dropped = 0;    // 因缓冲满而被丢弃的最旧帧数
    std::uint64_t empty = 0;      // 被丢弃的空帧数（不发布）
    std::size_t buffered = 0;     // 快照时刻缓冲区里的帧数
  };

  explicit CameraSession(std::unique_ptr<FrameSource> source);
  ~CameraSession();

  CameraSession(const CameraSession&) = delete;
  CameraSession& operator=(const CameraSession&) = delete;
  CameraSession(CameraSession&&) = delete;
  CameraSession& operator=(CameraSession&&) = delete;

  // 打开设备并启动工作线程。
  // 设备打不开时返回 false 且保持 Idle；已在 Running 时抛 std::logic_error。
  [[nodiscard]] bool start();

  // 停止并 join 工作线程，然后关闭设备。可重复调用，也可在从未启动时调用。
  void stop() noexcept;

  [[nodiscard]] bool is_running() const noexcept;

  // 等待一帧，最多 timeout 毫秒。超时、已停止或后台已失败时返回 false，
  // 调用方因此可以周期性检查 has_error() 并保持窗口响应。
  [[nodiscard]] bool wait_for_frame(Frame& frame,
                                    std::chrono::milliseconds timeout);

  [[nodiscard]] bool has_error() const noexcept;

  // 取出并清除后台异常；没有异常时返回空指针。
  std::exception_ptr take_error();

  [[nodiscard]] Stats stats() const;

  [[nodiscard]] const std::string& description() const noexcept {
    return description_;
  }

 private:
  void worker_loop() noexcept;
  void publish(cv::Mat frame);
  void record_error(std::exception_ptr error) noexcept;

  const std::unique_ptr<FrameSource> source_;
  const std::string description_;

  mutable std::mutex mutex_;
  std::condition_variable frame_ready_;
  std::deque<Frame> buffer_;
  Stats stats_{};
  std::exception_ptr error_;
  bool stop_requested_ = false;

  std::atomic<bool> running_{false};
  std::thread worker_;
};

}  // namespace camera
