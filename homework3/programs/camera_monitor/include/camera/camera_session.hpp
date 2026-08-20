#pragma once

#include "camera/frame_source.hpp"

#include <opencv2/core.hpp>

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <exception>
#include <memory>
#include <mutex>
#include <thread>

namespace sturdy_guide::camera {

// 独占一个 FrameSource，管理后台采集线程和最新帧的有界缓冲。
//
// 状态机：idle --start()--> running --stop()/读取失败/异常--> stopping
//          --线程退出并被 join()--> idle
// start() 与 wait() 由控制线程调用；stop() 与 is_running() 可被其他线程调用。
class CameraSession {
 public:
  explicit CameraSession(std::unique_ptr<FrameSource> source);
  ~CameraSession();

  CameraSession(const CameraSession&) = delete;
  CameraSession& operator=(const CameraSession&) = delete;

  // 启动后台线程。已在运行时会抛出 std::logic_error。
  void start();

  // 请求停止后台线程。可重复调用；只设置标志并唤醒，不阻塞。
  void stop();

  [[nodiscard]] bool is_running() const;

  // 非阻塞地取最新一帧。无新帧时返回 false。
  // 返回 true 时写入 image（采集到的画面）和 frame_number（采集序号）。
  bool try_latest_frame(cv::Mat& image, std::size_t& frame_number);

  // 等待后台线程结束并 join()。若后台发生读取异常，在此重新抛出。
  void wait();

 private:
  struct Frame {
    std::size_t index;
    cv::Mat image;
  };

  void run();

  std::unique_ptr<FrameSource> source_;
  mutable std::mutex mutex_;
  std::condition_variable state_changed_;
  std::thread worker_;
  std::exception_ptr error_;
  std::deque<Frame> frames_;  // 有界：最多保留两帧。
  bool stop_requested_ = false;
  bool running_ = false;
};

}  // namespace sturdy_guide::camera
