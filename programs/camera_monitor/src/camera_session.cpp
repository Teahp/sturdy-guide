#include "camera/camera_session.h"

#include <stdexcept>
#include <utility>

namespace camera {

CameraSession::CameraSession(std::unique_ptr<FrameSource> source)
    : source_(std::move(source)) {
  if (!source_) {
    throw std::invalid_argument("CameraSession requires a FrameSource");
  }
}

CameraSession::~CameraSession() { stop(); }

void CameraSession::start() {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (state_ == State::Running) {
      return;  // 幂等：已经在运行
    }
    // 可重启：清掉上一次的错误、缓冲与计数。
    error_ = nullptr;
    stop_requested_ = false;
    frames_.clear();
    frame_count_ = 0;
    state_ = State::Running;
  }
  // 状态已置 Running 且 stop() 只能通过 stop() 离开 Running，
  // 因此这里不可能同时存在两个 worker 线程。
  worker_ = std::thread(&CameraSession::run, this);
}

void CameraSession::stop() {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (state_ != State::Running) {
      return;  // 幂等：从未启动或已停止
    }
    stop_requested_ = true;
  }
  frame_available_.notify_all();  // 唤醒可能在 wait_frame 中等待的线程

  if (worker_.joinable()) {
    worker_.join();  // 等待后台线程退出；若因错误已退出则立即返回
  }

  {
    std::lock_guard<std::mutex> lock(mutex_);
    state_ = State::Stopped;
  }
  source_->release();  // 慢速 I/O 放在锁外；join 之后无并发访问
}

CameraSession::State CameraSession::state() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return state_;
}

bool CameraSession::wait_frame(cv::Mat& out,
                               std::chrono::milliseconds timeout) {
  std::unique_lock<std::mutex> lock(mutex_);
  frame_available_.wait_for(lock, timeout, [this] {
    return !frames_.empty() || stop_requested_ || error_ != nullptr;
  });
  if (frames_.empty()) {
    return false;  // 超时、已停止或出错，无帧可取
  }
  // 消费者只拿最新帧；缓冲中更旧的帧一并丢弃。
  out = frames_.back();
  frames_.clear();
  return true;
}

bool CameraSession::try_frame(cv::Mat& out) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (frames_.empty()) {
    return false;
  }
  out = frames_.back();
  frames_.clear();
  return true;
}

std::exception_ptr CameraSession::take_error() {
  std::lock_guard<std::mutex> lock(mutex_);
  return std::exchange(error_, nullptr);
}

std::size_t CameraSession::frame_count() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return frame_count_;
}

std::size_t CameraSession::buffered_frames() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return frames_.size();
}

void CameraSession::run() {
  try {
    if (!source_->open()) {
      throw std::runtime_error(
          "cannot open camera; check the device index, permissions, and "
          "whether another program is using it");
    }
    for (;;) {
      cv::Mat frame;
      const bool ok = source_->read(frame);  // 慢速 I/O，锁外执行

      bool should_stop = false;
      bool fatal = false;
      {
        std::lock_guard<std::mutex> lock(mutex_);
        if (stop_requested_) {
          should_stop = true;
        } else if (!ok) {
          fatal = true;  // 流结束视为错误；稍后在锁外抛出
        } else if (!frame.empty()) {
          frames_.push_back(std::move(frame));
          ++frame_count_;
          if (frames_.size() > kMaxBufferedFrames) {
            frames_.pop_front();  // 满则丢弃最旧帧
          }
        }
        // frame.empty() 为空帧：不发布为有效画面，继续循环。
      }

      if (should_stop) {
        break;
      }
      if (fatal) {
        throw std::runtime_error(
            "camera source stopped returning valid frames");
      }
      frame_available_.notify_all();
    }
  } catch (...) {
    // 后台异常：记录到 error_ 供主线程观察，并请求停止。
    std::lock_guard<std::mutex> lock(mutex_);
    error_ = std::current_exception();
    stop_requested_ = true;
  }
  frame_available_.notify_all();  // 唤醒 wait_frame：停止/出错后返回 false
}

}  // namespace camera
