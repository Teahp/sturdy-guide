#pragma once

#include "camera/frame_source.hpp"

#include <cstddef>
#include <deque>
#include <exception>
#include <memory>
#include <mutex>
#include <thread>

namespace sturdy_guide::camera {

// 一帧画面：帧号由采集会话递增分配，便于叠加与测试。
struct Frame {
  std::size_t sequence = 0;
  cv::Mat image;
};

// CameraSession 独占一个 FrameSource，管理一条后台采集线程。
//
// 线程安全契约（详见 README）：
// - start()/stop()/take_latest() 可在主线程调用；后台只有 run() 一条线程；
// - mutex_ 保护 buffer_、stop_requested_、running_、next_sequence_、
//   capture_error_ 与 worker_ 的生命周期；
// - 读帧（source_->read()）在锁外执行，避免慢 I/O 阻塞 stop()；
// - stop() 可重复调用；析构会先请求停止再 join 工作线程。
class CameraSession {
 public:
  explicit CameraSession(std::unique_ptr<FrameSource> source);
  ~CameraSession();

  CameraSession(const CameraSession&) = delete;
  CameraSession& operator=(const CameraSession&) = delete;

  // 启动后台采集。已在运行（或上一次线程尚未 join）时抛 std::logic_error。
  void start();

  // 请求停止并等待工作线程结束。可重复调用，无副作用。
  void stop();

  // 从缓冲取走最新一帧（主线程调用）。有新帧返回 true 并写入 out；
  // 没有则返回 false。取走时清空缓冲：更旧的帧已过时，直接丢弃。
  bool take_latest(Frame& out);

  // 当前缓冲待显示的帧数（用于测试断言不超过两帧）。
  std::size_t buffered_frames() const;

  // 取出并清空后台采集错误；没有错误时返回 nullptr。
  std::exception_ptr take_error();

 private:
  void run();  // 工作线程主体：循环读帧 -> 放入有界缓冲。

  std::unique_ptr<FrameSource> source_;
  mutable std::mutex mutex_;
  std::thread worker_;

  bool running_ = false;         // 工作线程是否在运行
  bool stop_requested_ = false;  // 停止请求标志（牌子）
  std::deque<Frame> buffer_;     // 有界缓冲，最多保留最新两帧
  std::size_t next_sequence_ = 0;
  std::exception_ptr capture_error_;
};

}  // namespace sturdy_guide::camera
