#pragma once

#include "camera/frame_source.hpp"

#include <cstddef>
#include <cstdint>
#include <deque>
#include <exception>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>

namespace sturdy_guide::camera {

struct CapturedFrame {
  cv::Mat image;
  std::uint64_t sequence = 0;
  double capture_fps = 0.0;
};//捕获帧结构体

enum class CameraSessionState {
  Ready,
  Running,
  Stopping,
  Stopped,
  Failed,
};//状态机

class CameraSession {
 public:
  explicit CameraSession(std::unique_ptr<FrameSource> source);//构造函数，独占摄像头源，传入一个 FrameSource 的独占指针，表示摄像头会话将使用该源来捕获帧。
  ~CameraSession();//析构函数，仅仅调用 stop()，确保在销毁 CameraSession 对象时，后台线程被正确停止，并释放相关资源。

  CameraSession(const CameraSession&) = delete;//禁止拷贝构造函数
  CameraSession& operator=(const CameraSession&) = delete;//禁止拷贝赋值运算符

  //启动摄像头会话，开始捕获帧。
  //两把锁，生命周期锁和状态锁，确保在状态机转换时，不会出现竞争条件。
  void start();

  // 幂等和线程安全
  void stop();

  // Returns the freshest image and discards older pending images.
  [[nodiscard]] std::optional<CapturedFrame> try_take_latest();
  [[nodiscard]] std::size_t buffered_frame_count() const;
  [[nodiscard]] CameraSessionState state() const;

  // Rethrows a non-shutdown error captured by the worker thread, if any.
  void rethrow_if_error() const;

 private:
  void join_worker();
  void run();

  std::unique_ptr<FrameSource> source_;
  mutable std::mutex mutex_;//互斥锁，用于保护共享数据
  std::mutex lifecycle_mutex_;//生命周期互斥锁，start() 和 stop() 方法需要获取该锁，确保不会交叉调用
  std::thread worker_;
  std::deque<CapturedFrame> frames_;//双端队列，用于存储捕获的帧
  std::exception_ptr worker_error_;//异常指针，用于存储捕获的异常
  bool stop_requested_ = false;//停止请求标志，这里是bool类型，只在一个线程中使用，所以不需要是原子类型
  std::uint64_t next_sequence_ = 0;//下一个序列号，用于标识帧
  CameraSessionState state_ = CameraSessionState::Ready;//设置初始状态为Ready
};

}  // namespace sturdy_guide::camera
