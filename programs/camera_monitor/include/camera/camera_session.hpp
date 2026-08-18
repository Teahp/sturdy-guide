#ifndef CAMERA_CAMERA_SESSION_HPP
#define CAMERA_CAMERA_SESSION_HPP

#include "camera/frame_source.hpp"

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <exception>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

namespace camera {

// 采集会话：独占一个 FrameSource，在后台线程持续读帧，
// 维护最多 kMaxBufferedFrames 帧的有界最新帧缓冲，并把后台异常带回调用线程。
//
// 状态机（详细契约见 README）：
//   Idle -> Running -> Stopping -> Stopped -> Idle
//   Idle     未启动，或工作线程已 join，可 start()
//   Running  工作线程正在采集
//   Stopping 已请求停止，正在等待 join
//   Stopped  工作线程已退出但尚未 join（等待 stop() 完成 join）
class CameraSession {
 public:
  static constexpr std::size_t kMaxBufferedFrames = 2;

  explicit CameraSession(std::unique_ptr<FrameSource> source);
  ~CameraSession();

  CameraSession(const CameraSession&) = delete;
  CameraSession& operator=(const CameraSession&) = delete;

  // 从 Idle 启动后台线程；已在运行/停止中返回 false。
  bool start();

  // 停止并 join 工作线程。可重复调用：第一次真正执行，后续立即返回。
  void stop();

  // 等待一帧（最多 timeout 毫秒）。成功返回 true 并写入 out；
  // 超时、会话已停止或出错返回 false。只允许消费者线程调用。
  bool wait_for_frame(cv::Mat& out,
                      std::chrono::milliseconds timeout =
                          std::chrono::milliseconds{16});

  // 后台是否报告了错误。
  bool has_error() const;
  // 后台错误信息；没有错误时返回空字符串。
  std::string error_message() const;

  // 只读统计。工作线程停止后再读取即为确定值。
  std::size_t frames_produced() const;
  std::size_t dropped_frames() const;
  std::size_t buffered_frames() const;

 private:
  void worker_loop();
  void deliver_frame(cv::Mat frame);
  void report_error(std::exception_ptr error);
  void finish_worker();
  bool should_stop() const;

  std::unique_ptr<FrameSource> source_;

  mutable std::mutex mutex_;
  std::condition_variable cv_;
  std::thread worker_;

  enum class State { Idle, Running, Stopping, Stopped };
  State state_ = State::Idle;

  std::deque<cv::Mat> frames_;
  std::size_t frames_produced_ = 0;
  std::size_t dropped_frames_ = 0;

  std::exception_ptr background_error_;
  bool error_reported_ = false;
};

}  // namespace camera

#endif  // CAMERA_CAMERA_SESSION_HPP
