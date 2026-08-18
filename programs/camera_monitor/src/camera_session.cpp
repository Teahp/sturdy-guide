#include "camera/camera_session.hpp"

#include <opencv2/core.hpp>
#include <stdexcept>
#include <utility>

namespace camera {

CameraSession::CameraSession(std::unique_ptr<FrameSource> source)
    : source_(std::move(source)) {}

CameraSession::~CameraSession() {
  // 析构时先停止并 join 工作线程，保证线程先于对象销毁。
  stop();
}

bool CameraSession::start() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (state_ != State::Idle) {
    return false;  // 已在运行或停止中，不允许重复启动
  }

  state_ = State::Running;
  frames_.clear();
  frames_produced_ = 0;
  dropped_frames_ = 0;
  background_error_ = nullptr;
  error_reported_ = false;

  worker_ = std::thread(&CameraSession::worker_loop, this);
  return true;
}

void CameraSession::stop() {
  std::unique_lock<std::mutex> lock(mutex_);
  if (state_ == State::Idle || state_ == State::Stopping) {
    return;  // 已经停止，或另一个线程正在执行停止；重复调用安全
  }

  state_ = State::Stopping;
  cv_.notify_all();  // 唤醒等待帧的消费者，让它们尽快退出
  lock.unlock();

  // join 必须在锁外执行：持锁 join 会与 worker 抢锁形成死锁。
  if (worker_.joinable()) {
    worker_.join();
  }

  lock.lock();
  state_ = State::Idle;
  cv_.notify_all();
}

bool CameraSession::wait_for_frame(cv::Mat& out,
                                   std::chrono::milliseconds timeout) {
  std::unique_lock<std::mutex> lock(mutex_);
  static_cast<void>(cv_.wait_for(lock, timeout, [this] {
    return !frames_.empty() || state_ == State::Stopped ||
           state_ == State::Idle;
  }));

  if (frames_.empty()) {
    return false;  // 超时，或会话已停止/出错
  }
  out = frames_.front();  // cv::Mat 拷贝是浅拷贝（引用计数），代价很低
  frames_.pop_front();
  return true;
}

bool CameraSession::has_error() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return error_reported_;
}

std::string CameraSession::error_message() const {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!background_error_) {
    return {};
  }
  try {
    std::rethrow_exception(background_error_);
  } catch (const std::exception& error) {
    return error.what();
  } catch (...) {
    return "unknown camera error";
  }
}

std::size_t CameraSession::frames_produced() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return frames_produced_;
}

std::size_t CameraSession::dropped_frames() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return dropped_frames_;
}

std::size_t CameraSession::buffered_frames() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return frames_.size();
}

void CameraSession::worker_loop() {
  // 先检查是否在启动完成前就被要求停止。
  if (should_stop()) {
    finish_worker();
    return;
  }

  if (!source_->open()) {
    report_error(std::make_exception_ptr(std::runtime_error(
        "cannot open camera; check the device index, permissions, and "
        "whether another program is using it")));
    finish_worker();
    return;
  }

  while (!should_stop()) {
    cv::Mat frame;
    try {
      if (!source_->read(frame)) {
        report_error(std::make_exception_ptr(
            std::runtime_error("camera stopped returning valid frames")));
        break;
      }
      if (frame.empty()) {
        report_error(std::make_exception_ptr(
            std::runtime_error("camera produced an empty frame")));
        break;
      }
    } catch (...) {
      // 后台异常通过 exception_ptr 越过线程边界，交给调用线程观察。
      report_error(std::current_exception());
      break;
    }
    deliver_frame(std::move(frame));
  }

  source_->close();
  finish_worker();
}

bool CameraSession::should_stop() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return state_ == State::Stopping || state_ == State::Stopped;
}

void CameraSession::deliver_frame(cv::Mat frame) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (state_ != State::Running) {
    return;  // 正在停止：不再入队，直接丢弃
  }

  ++frames_produced_;
  if (frames_.size() >= kMaxBufferedFrames) {
    // 缓冲已满：丢弃最旧帧，保留最新帧，缓冲永不无限增长。
    frames_.pop_front();
    ++dropped_frames_;
  }
  frames_.push_back(std::move(frame));
  cv_.notify_one();
}

void CameraSession::report_error(std::exception_ptr error) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!background_error_) {
    background_error_ = std::move(error);
  }
  error_reported_ = true;
  cv_.notify_all();
}

void CameraSession::finish_worker() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (state_ == State::Running || state_ == State::Stopping) {
    state_ = State::Stopped;
  }
  cv_.notify_all();
}

}  // namespace camera
