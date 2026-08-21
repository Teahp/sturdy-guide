#pragma once

#include "camera/frame_source.hpp"

#include <exception>
#include <memory>
#include <mutex>
#include <thread>
#include <optional>

#include <opencv2/core.hpp>

namespace sturdy_guide::camera {

// CameraSession owns the frame source, the background capture thread,
// and a thread-safe frame buffer that holds at most two frames.
//
// Thread model:
//   - The background worker calls FrameSource::read() in a loop.
//   - The main thread reads the latest frame via latest_frame().
//
// Lifecycle:
//   start()  -> launches the worker thread
//   stop()   -> sets a stop flag (non-blocking)
//   join()   -> blocks until the worker thread has exited
//
// After join() returns the session is idle and may be restarted.
class CameraSession {
 public:
  explicit CameraSession(std::unique_ptr<FrameSource> source);
  ~CameraSession();

  CameraSession(const CameraSession&) = delete;
  CameraSession& operator=(const CameraSession&) = delete;

  // Launches the background capture thread. Throws std::logic_error
  // if the session is already running.
  void start();

  // Signals the background thread to stop. Safe to call from any thread.
  void stop();

  // Blocks until the background thread has exited. After join() the
  // session is idle.
  void join();

  // Returns true while the background thread is running.
  [[nodiscard]] bool is_running() const;

  // Returns the most recent frame, or std::nullopt if no frame is
  // available yet. This is the only method the main thread should call.
  [[nodiscard]] std::optional<cv::Mat> latest_frame();

  // Returns an stored exception from the background thread, or
  // nullptr if no error occurred.
  [[nodiscard]] std::exception_ptr error() const;

 private:
  void run();

  std::unique_ptr<FrameSource> source_;
  mutable std::mutex mutex_;
  std::thread worker_;

  std::optional<cv::Mat> buffer_[2];
  bool buffer_dirty_[2] = {false, false};

  std::exception_ptr worker_error_;
  bool stop_requested_ = false;
  bool running_ = false;
};

}  // namespace sturdy_guide::camera
/*
独自持有 FrameSource + 收束后台线程 + 帧缓冲区（保留两帧） + 互斥锁，
  互斥锁保护的变量
    std::optional<cv::Mat> buffer_[2];      // 帧缓冲区双槽
    bool buffer_dirty_[2] = {false, false}; // 每个槽是否有有效帧
    std::exception_ptr worker_error_;       // 后台线程捕获的异常
    bool stop_requested_ = false;           // 停止信号
    bool running_ = false;                  // 后台线程是否在运行
    std::thread worker_;                    // 线程句柄（join时移动）
管理 start()/stop()/join()
 生命周期控制
 */