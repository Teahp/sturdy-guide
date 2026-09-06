#include "camera/camera_session.hpp"

#include <stdexcept>
#include <utility>

namespace sturdy_guide::camera {

CameraSession::CameraSession(std::unique_ptr<FrameSource> source)
    : source_(std::move(source)) {}

CameraSession::~CameraSession() {
  // 先请求停止并 join 工作线程，之后成员才被销毁，线程不会访问已析构对象。
  stop();
}

void CameraSession::start() {
  {
    std::lock_guard<std::mutex> lock{mutex_};
    if (running_ || worker_.joinable()) {
      throw std::logic_error("camera session is already started");
    }
    stop_requested_ = false;
    capture_error_ = nullptr;
    buffer_.clear();
    next_sequence_ = 0;
    running_ = true;
  }

  try {
    worker_ = std::thread(&CameraSession::run, this);
  } catch (...) {
    std::lock_guard<std::mutex> lock{mutex_};
    running_ = false;
    throw;
  }
}

void CameraSession::stop() {
  {
    std::lock_guard<std::mutex> lock{mutex_};
    stop_requested_ = true;
  }

  // 把工作线程移到局部变量再 join，避免持锁等待线程退出。
  std::thread worker;
  {
    std::lock_guard<std::mutex> lock{mutex_};
    if (worker_.joinable()) {
      worker = std::move(worker_);
    }
  }
  if (worker.joinable()) {
    worker.join();
  }
}

bool CameraSession::take_latest(Frame& out) {
  std::lock_guard<std::mutex> lock{mutex_};
  if (buffer_.empty()) {
    return false;
  }
  out = std::move(buffer_.back());
  buffer_.clear();  // 只留最新的；更旧的帧已过时，直接丢弃。
  return true;
}

std::size_t CameraSession::buffered_frames() const {
  std::lock_guard<std::mutex> lock{mutex_};
  return buffer_.size();
}

std::exception_ptr CameraSession::take_error() {
  std::lock_guard<std::mutex> lock{mutex_};
  return std::exchange(capture_error_, nullptr);
}

void CameraSession::run() {
  try {
    for (;;) {
      // 慢 I/O：读帧在锁外进行，避免阻塞 stop()。
      cv::Mat image = source_->read();

      std::lock_guard<std::mutex> lock{mutex_};
      if (stop_requested_) {
        break;  // 被要求停止：不再发布新帧。
      }
      if (image.empty()) {
        throw std::runtime_error("empty frame from camera source");
      }

      Frame frame;
      frame.sequence = next_sequence_++;
      frame.image = std::move(image);

      buffer_.push_back(std::move(frame));
      while (buffer_.size() > 2) {
        buffer_.pop_front();  // 缓冲有界：放不下就丢最旧的一帧。
      }
    }
  } catch (...) {
    std::lock_guard<std::mutex> lock{mutex_};
    capture_error_ = std::current_exception();  // 把后台异常留到主线程观察。
  }

  {
    std::lock_guard<std::mutex> lock{mutex_};
    running_ = false;
  }
}

}  // namespace sturdy_guide::camera
