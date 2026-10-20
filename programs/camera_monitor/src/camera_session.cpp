#include "camera/camera_session.hpp"

#include <stdexcept>
#include <utility>

namespace camera {

CameraSession::CameraSession(std::unique_ptr<FrameSource> source)
    : source_(std::move(source)),
      description_(source_ != nullptr ? source_->description() : std::string{}) {
  if (source_ == nullptr) {
    throw std::invalid_argument("camera session requires a frame source");
  }
}

CameraSession::~CameraSession() { stop(); }

bool CameraSession::start() {
  if (running_.load()) {
    throw std::logic_error("camera session is already running");
  }

  {
    std::lock_guard<std::mutex> lock{mutex_};
    stop_requested_ = false;
    error_ = nullptr;
    buffer_.clear();
    stats_ = Stats{};
  }

  // 设备在当前线程打不开时直接返回 false，错误信息可以同步报告给用户，
  // 不会变成"线程跑起来了但一帧都没有"这种难以定位的状态。
  if (!source_->open()) {
    return false;
  }

  running_.store(true);
  try {
    worker_ = std::thread{[this] { worker_loop(); }};
  } catch (...) {
    running_.store(false);
    source_->close();
    throw;
  }
  return true;
}

void CameraSession::stop() noexcept {
  running_.store(false);
  {
    std::lock_guard<std::mutex> lock{mutex_};
    stop_requested_ = true;
  }
  frame_ready_.notify_all();

  if (worker_.joinable()) {
    // 工作线程只在 read() 返回之后观察停止标志，因此这里等待的是一次读帧的耗时。
    worker_.join();
  }

  // 线程已经退出，close() 不会与 read() 并发；FrameSource::close() 约定可重复调用。
  source_->close();
}

bool CameraSession::is_running() const noexcept { return running_.load(); }

bool CameraSession::wait_for_frame(Frame& frame,
                                   std::chrono::milliseconds timeout) {
  std::unique_lock<std::mutex> lock{mutex_};
  frame_ready_.wait_for(lock, timeout, [this] {
    return !buffer_.empty() || stop_requested_ || error_ != nullptr;
  });

  if (buffer_.empty()) {
    return false;
  }

  frame = std::move(buffer_.front());
  buffer_.pop_front();
  ++stats_.delivered;
  return true;
}

bool CameraSession::has_error() const noexcept {
  std::lock_guard<std::mutex> lock{mutex_};
  return error_ != nullptr;
}

std::exception_ptr CameraSession::take_error() {
  std::lock_guard<std::mutex> lock{mutex_};
  return std::exchange(error_, nullptr);
}

CameraSession::Stats CameraSession::stats() const {
  std::lock_guard<std::mutex> lock{mutex_};
  Stats snapshot = stats_;
  snapshot.buffered = buffer_.size();
  return snapshot;
}

void CameraSession::worker_loop() noexcept {
  try {
    cv::Mat raw;
    for (;;) {
      {
        std::lock_guard<std::mutex> lock{mutex_};
        if (stop_requested_) {
          return;
        }
      }

      // read() 在锁外执行：摄像头可能阻塞几十毫秒，绝不能占着会话锁。
      raw = cv::Mat{};
      if (!source_->read(raw)) {
        std::lock_guard<std::mutex> lock{mutex_};
        if (stop_requested_) {
          return;
        }
        throw std::runtime_error("camera stopped returning valid frames");
      }

      if (raw.empty()) {
        std::lock_guard<std::mutex> lock{mutex_};
        if (stop_requested_) {
          return;
        }
        ++stats_.empty;
        continue;
      }

      publish(std::move(raw));
    }
  } catch (...) {
    // 后台异常不在这里打印，而是转存后由调用线程决定如何报告，
    // 保证 stderr 的写入仍然只发生在主线程。
    record_error(std::current_exception());
  }
}

void CameraSession::publish(cv::Mat frame) {
  std::unique_lock<std::mutex> lock{mutex_};
  ++stats_.captured;

  Frame next;
  next.image = std::move(frame);
  next.sequence = stats_.captured;
  next.captured_at = std::chrono::steady_clock::now();

  if (buffer_.size() >= kMaxBufferedFrames) {
    // 预览只关心最新画面：缓冲满时丢最旧的一帧，而不是丢最新的一帧。
    buffer_.pop_front();
    ++stats_.dropped;
  }
  buffer_.push_back(std::move(next));

  lock.unlock();
  frame_ready_.notify_one();
}

void CameraSession::record_error(std::exception_ptr error) noexcept {
  {
    std::lock_guard<std::mutex> lock{mutex_};
    if (error_ == nullptr) {
      error_ = std::move(error);
    }
  }
  running_.store(false);
  // 唤醒可能正阻塞在 wait_for_frame() 的调用线程，让它尽快看到失败。
  frame_ready_.notify_all();
}

}  // namespace camera
