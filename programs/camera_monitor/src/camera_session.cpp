#include "camera/camera_session.hpp"

#include <exception>
#include <utility>

namespace camera {

CameraSession::CameraSession(std::unique_ptr<FrameSource> source)
    : source_(std::move(source)) {}

CameraSession::~CameraSession() {
  // 析构必须先 join 工作线程，再释放成员；stop() 是幂等的。
  stop();
}

void CameraSession::start() {
  {
    std::unique_lock lock(mutex_);
    if (state_ == State::kRunning || state_ == State::kStopping) {
      return;  // 已运行或正在停止：无操作
    }
    // Idle 或 Stopped：开启一次新的采集运行，清空上次的错误与统计。
    error_.clear();
    stop_requested_ = false;
    head_ = 0;
    count_ = 0;
    newest_sequence_ = 0;
    captured_ = 0;
    dropped_ = 0;
  }

  // 在调用线程打开设备；失败时异常同步传播，状态保持 Idle/Stopped。
  source_->open();

  {
    std::unique_lock lock(mutex_);
    state_ = State::kRunning;
    worker_ = std::thread(&CameraSession::workerLoop, this);
  }
}

void CameraSession::stop() {
  // 串行化并发 stop()/析构：只有第一个调用者执行 join。
  std::unique_lock lifecycle(lifecycle_mutex_);
  {
    std::unique_lock lock(mutex_);
    if (state_ == State::kIdle || state_ == State::kStopped) {
      return;  // 从未启动或已停止：无操作
    }
    state_ = State::kStopping;
    stop_requested_ = true;
  }
  frame_cv_.notify_all();  // 唤醒可能阻塞在 waitForFrame() 的消费者
  if (worker_.joinable()) {
    worker_.join();
  }
  {
    std::unique_lock lock(mutex_);
    state_ = State::kStopped;
  }
}

bool CameraSession::running() const {
  std::unique_lock lock(mutex_);
  return state_ == State::kRunning;
}

bool CameraSession::tryTakeFrame(cv::Mat& out, std::size_t* sequence_out) {
  std::unique_lock lock(mutex_);
  return takeLocked(out, sequence_out);
}

bool CameraSession::waitForFrame(cv::Mat& out,
                                 const std::chrono::milliseconds timeout,
                                 std::size_t* sequence_out) {
  std::unique_lock lock(mutex_);
  frame_cv_.wait_for(lock, timeout,
                     [this] { return count_ > 0 || stop_requested_; });
  return takeLocked(out, sequence_out);
}

std::string CameraSession::error() const {
  std::unique_lock lock(mutex_);
  return error_;
}

std::size_t CameraSession::capturedFrames() const {
  std::unique_lock lock(mutex_);
  return captured_;
}

std::size_t CameraSession::droppedFrames() const {
  std::unique_lock lock(mutex_);
  return dropped_;
}

void CameraSession::workerLoop() {
  std::string failure;
  try {
    for (;;) {
      {
        std::unique_lock lock(mutex_);
        if (stop_requested_) {
          return;  // 正常停止：不发布任何帧
        }
      }

      // 摄像头读取位于会话 mutex 之外：慢速 I/O 不得占用临界区。
      cv::Mat frame;
      const bool ok = source_->read(frame);
      if (!ok) {
        failure = "camera stopped returning valid frames";
        break;
      }
      if (frame.empty()) {
        // 空帧不会被发布为有效画面。
        failure = "camera stopped returning valid frames";
        break;
      }

      {
        std::unique_lock lock(mutex_);
        if (stop_requested_) {
          return;  // 读取期间收到了停止请求：丢弃这一帧
        }
        publishLocked(std::move(frame));
      }
      frame_cv_.notify_all();
    }
  } catch (const std::exception& error) {
    failure = error.what();
  } catch (...) {
    failure = "unknown error in capture thread";
  }

  // 后台异常越过线程边界：存入 error_（保留首个），唤醒消费者，
  // 并把状态置为 Stopping，等待 stop()/析构完成 join。
  {
    std::unique_lock lock(mutex_);
    if (error_.empty()) {
      error_ = std::move(failure);
    }
    stop_requested_ = true;
    state_ = State::kStopping;
  }
  frame_cv_.notify_all();
}

void CameraSession::publishLocked(cv::Mat&& frame) {
  // 缓冲满（2 帧未消费）时丢弃最旧的一帧，让位给最新帧。
  // 丢弃旧帧保证队列不增长，且显示总是拿到最新画面。
  if (count_ == kBufferCapacity) {
    head_ = (head_ + 1) % kBufferCapacity;
    --count_;
    ++dropped_;
  }
  const std::size_t slot = (head_ + count_) % kBufferCapacity;
  frames_[slot] = std::move(frame);
  ++count_;
  newest_sequence_ = ++captured_;
}

bool CameraSession::takeLocked(cv::Mat& out, std::size_t* sequence_out) {
  if (count_ == 0) {
    return false;
  }
  const std::size_t slot = (head_ + count_ - 1) % kBufferCapacity;
  // cv::Mat 是引用计数句柄：out 与缓冲共享像素数据，缓冲被覆盖后 out 仍有效。
  out = frames_[slot];
  if (sequence_out != nullptr) {
    *sequence_out = newest_sequence_;
  }
  // 只保留最新帧；其余旧帧一并丢弃（已过时的画面无需再显示）。
  count_ = 0;
  return true;
}

}  // namespace camera
