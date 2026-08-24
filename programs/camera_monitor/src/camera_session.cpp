#include "camera/camera_session.hpp"

#include <chrono>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>

namespace sturdy_guide::camera {

CameraSession::CameraSession(std::unique_ptr<FrameSource> source)
    : source_(std::move(source)) {}

CameraSession::~CameraSession() { stop(); }

void CameraSession::start(int device, int width, int height) {
  std::thread leftover;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (state_ == State::kRunning) return;  // 已运行：no-op
    if (state_ == State::kStopping) {
      throw std::logic_error("cannot start while session is stopping");
    }
    // 上一任线程可能已自行退出但尚未 join；先收编再重启。
    if (worker_.joinable()) leftover = std::move(worker_);
  }
  if (leftover.joinable()) leftover.join();

  // 打开设备属于慢速 I/O，放在锁外。
  if (!source_->open(device, width, height)) {
    throw std::runtime_error(source_->lastError());
  }

  {
    std::lock_guard<std::mutex> lock(mutex_);
    state_ = State::kRunning;
    ready_ = -1;
    capture_fps_ = 0.0;
    error_ = nullptr;
    try {
      worker_ = std::thread(&CameraSession::run, this);
    } catch (...) {
      state_ = State::kStopped;
      throw;
    }
  }
}

void CameraSession::stop() {
  std::thread to_join;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (state_ == State::kStopping) return;  // 已有线程在 join
    if (state_ == State::kRunning) state_ = State::kStopping;
    // 无论工作线程是仍在运行还是已自行退出，只要尚未 join 就收编，
    // 否则析构 joinable 的 std::thread 会触发 std::terminate。
    if (worker_.joinable()) to_join = std::move(worker_);
  }
  frame_ready_.notify_all();
  // join 必须在锁外：工作线程退出前还要拿锁更新 state_，
  // 持锁 join 会与之互相等待而死锁。
  if (to_join.joinable()) to_join.join();
}

bool CameraSession::isRunning() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return state_ == State::kRunning;
}

bool CameraSession::tryTakeFrame(Frame& out) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (ready_ < 0) return false;
  out = slots_[ready_];
  ready_ = -1;
  return true;
}

bool CameraSession::hasError() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return static_cast<bool>(error_);
}

void CameraSession::rethrowError() const {
  std::lock_guard<std::mutex> lock(mutex_);
  if (error_) std::rethrow_exception(error_);
}

double CameraSession::captureFps() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return capture_fps_;
}

void CameraSession::fail(std::exception_ptr error) {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!error_) error_ = std::move(error);  // 只保留首个错误
  }
  frame_ready_.notify_all();
}

void CameraSession::run() {
  std::size_t frames_in_window = 0;
  auto rate_started_at = std::chrono::steady_clock::now();
  double fps = 0.0;
  int write_slot = 0;

  try {
    while (true) {
      // 直接写入 worker 独占的槽位：读帧（慢 I/O）全程不持锁。
      Frame& slot = slots_[write_slot];
      if (!source_->read(slot)) {
        std::string message = source_->lastError();
        if (message.empty()) message = "camera stopped returning valid frames";
        fail(std::make_exception_ptr(std::runtime_error(message)));
        break;
      }

      const bool publish = !slot.image.empty();

      if (publish) {
        ++frames_in_window;
        const auto now = std::chrono::steady_clock::now();
        const auto window = now - rate_started_at;
        if (window >= std::chrono::seconds{1}) {
          fps = static_cast<double>(frames_in_window) /
                std::chrono::duration<double>{window}.count();
          frames_in_window = 0;
          rate_started_at = now;
        }
      }

      {
        std::lock_guard<std::mutex> lock(mutex_);
        if (state_ != State::kRunning) break;  // 停止请求；空帧也检查停止
        if (publish) {
          capture_fps_ = fps;
          ready_ = write_slot;        // 发布当前槽位
          write_slot = 1 - write_slot;
        }
      }
      if (publish) frame_ready_.notify_one();
    }
  } catch (...) {
    fail(std::current_exception());
  }

  {
    std::lock_guard<std::mutex> lock(mutex_);
    state_ = State::kStopped;
  }
  frame_ready_.notify_all();
}

}  // namespace sturdy_guide::camera
