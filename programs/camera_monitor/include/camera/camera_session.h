// CameraSession：独占一个 FrameSource，管理采集线程生命周期与有界帧缓冲。
//
// 线程模型：
//  - 后台采集线程：循环调用 source_->read()，把最新帧发布进有界缓冲；
//  - 控制/消费线程（通常主线程）：start()/stop()/wait_frame()/take_error()。
// 慢速 I/O（read/open/release）永远在锁外执行；锁只保护共享状态的完整转换。
#pragma once

#include "camera/frame_source.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <exception>
#include <memory>
#include <mutex>
#include <thread>

namespace camera {

class CameraSession {
 public:
  enum class State { Idle, Running, Stopped };

  // 有界缓冲容量：最多保留两帧（双缓冲），满时丢弃最旧帧。
  static constexpr std::size_t kMaxBufferedFrames = 2;

  // 取得帧源所有权；帧源在会话运行期间只被后台线程读写。
  explicit CameraSession(std::unique_ptr<FrameSource> source);
  ~CameraSession();  // 等价于 stop()：置停止标志 → notify → join → release

  CameraSession(const CameraSession&) = delete;
  CameraSession& operator=(const CameraSession&) = delete;

  // 启动采集线程。幂等：已在 Running 时为 no-op。
  // Idle/Stopped → Running。帧源打开失败或读取失败时，后台线程记录错误后
  // 退出，主线程可通过 take_error() 观察；之后调用 stop() 完成收尾即可重启。
  void start();

  // 请求停止并 join() 后台线程。幂等：任意状态调用都安全，可连续调用。
  // 注意：不要在后台线程自身（如 FrameSource::read 内部）调用。
  void stop();

  State state() const;

  // 消费最新帧：最多等待 timeout。有帧返回 true 并复制到 out，同时清空缓冲
  // （消费者只拿最新一帧，更旧的帧被丢弃）。线程安全，可被任意线程调用。
  bool wait_frame(cv::Mat& out, std::chrono::milliseconds timeout);

  // 非阻塞取最新帧。线程安全。
  bool try_frame(cv::Mat& out);

  // 取出并清除后台异常；无异常返回 nullptr。线程安全。
  std::exception_ptr take_error();

  // 后台线程已成功采集（发布进缓冲）的帧总数，单调递增。线程安全。
  std::size_t frame_count() const;

  // 当前缓冲中的帧数（诊断/测试用，恒 <= kMaxBufferedFrames）。线程安全。
  std::size_t buffered_frames() const;

 private:
  void run();  // 后台线程入口

  std::unique_ptr<FrameSource> source_;
  mutable std::mutex mutex_;
  std::condition_variable frame_available_;
  std::deque<cv::Mat> frames_;     // 有界：最多 kMaxBufferedFrames 帧
  std::exception_ptr error_;       // 后台异常（跨线程回传）
  bool stop_requested_ = false;    // 停止标志
  State state_ = State::Idle;
  std::size_t frame_count_ = 0;
  std::thread worker_;
};

}  // namespace camera
