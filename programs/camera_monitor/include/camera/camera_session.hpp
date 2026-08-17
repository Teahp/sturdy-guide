#pragma once

#include "camera/frame_source.hpp"

#include <array>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

namespace camera {

// 采集会话：独占一个 FrameSource，在后台线程持续读取，并把最新帧
// 交给调用线程，同时把后台异常传回调用线程。
//
// 状态机（全部转换在 mutex 保护下进行）：
//   Idle --start()--> Running            （open() 失败则保持 Idle 并抛出）
//   Running --stop()--> Stopping --join--> Stopped
//   Running --后台失败--> Stopping --join--> Stopped（error() 非空）
//   Running --start()--> 无操作（幂等）
//   Stopped --start()--> Running         （清空上次的错误与统计）
//   Stopped / Idle --stop()--> 无操作（幂等）
//
// 线程安全契约（详见 README.md）：
//   - start()/stop() 供单个调用方（主线程）顺序调用；stop() 可重复调用，
//     也允许从任意线程调用，但不要与 start() 并发。
//   - tryTakeFrame()/waitForFrame()/running()/error()/统计接口可从任意线程调用。
//   - 慢速 I/O（设备读取、imshow、imwrite）永不发生在会话的 mutex 临界区内。
class CameraSession {
 public:
  // 缓冲容量：内存中最多保留两帧，显示落后时丢弃旧帧。
  static constexpr std::size_t kBufferCapacity = 2;

  enum class State { kIdle, kRunning, kStopping, kStopped };

  explicit CameraSession(std::unique_ptr<FrameSource> source);
  ~CameraSession();

  CameraSession(const CameraSession&) = delete;
  CameraSession& operator=(const CameraSession&) = delete;

  // 打开设备并启动后台采集线程。设备打开失败时抛出 std::runtime_error，
  // 异常同步传播，会话保持 Idle/Stopped，不会创建线程。
  // 已处于 Running/Stopping 时调用是无操作（幂等）。
  void start();

  // 请求停止、唤醒并 join() 工作线程。重复调用安全；未启动时也是无操作。
  void stop();

  bool running() const;

  // 取走最新一帧（非阻塞）。没有新帧返回 false。
  // 取走后缓冲清空：只保留最新帧，其余旧帧按约定丢弃。
  // sequence_out 接收该帧的采集序号（从 1 开始递增）。
  bool tryTakeFrame(cv::Mat& out, std::size_t* sequence_out = nullptr);

  // 阻塞等待最新一帧，最多等待 timeout。超时、已停止或后台失败时返回 false。
  bool waitForFrame(cv::Mat& out, std::chrono::milliseconds timeout,
                    std::size_t* sequence_out = nullptr);

  // 后台线程最近一次错误；空串表示无错误。首个错误被保留。
  std::string error() const;

  std::size_t capturedFrames() const;  // 成功读取并发布的帧数（= 最新序号）
  std::size_t droppedFrames() const;   // 因缓冲满被丢弃的帧数

 private:
  void workerLoop();
  void publishLocked(cv::Mat&& frame);
  bool takeLocked(cv::Mat& out, std::size_t* sequence_out);

  std::unique_ptr<FrameSource> source_;

  mutable std::mutex mutex_;
  std::mutex lifecycle_mutex_;  // 串行化 stop()/析构，保证 join 恰好一次
  std::condition_variable frame_cv_;

  std::thread worker_;
  State state_ = State::kIdle;
  bool stop_requested_ = false;
  std::string error_;

  // 有界环形缓冲：容量 2，始终保留最新的 1–2 帧。
  std::array<cv::Mat, kBufferCapacity> frames_;
  std::size_t head_ = 0;   // 最旧未消费帧的下标
  std::size_t count_ = 0;  // 未消费帧数（0..2）
  std::size_t newest_sequence_ = 0;

  std::size_t captured_ = 0;  // 成功读取并发布的帧数
  std::size_t dropped_ = 0;   // 因缓冲满被丢弃的帧数
};

}  // namespace camera
