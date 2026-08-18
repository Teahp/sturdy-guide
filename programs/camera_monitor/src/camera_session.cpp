#include "camera/camera_session.hpp"

#include <chrono>
#include <stdexcept>
#include <utility>

namespace sturdy_guide::camera {

CameraSession::CameraSession(std::unique_ptr<FrameSource> source)
    : source_(std::move(source)) {
  if (!source_) {
    throw std::invalid_argument("CameraSession requires a frame source");
  }
}

CameraSession::~CameraSession() { stop(); }

void CameraSession::start() {
  const std::scoped_lock lifecycle_lock{lifecycle_mutex_};
  {
    const std::scoped_lock lock{mutex_};
    if (state_ != CameraSessionState::Ready) {
      throw std::logic_error("CameraSession can only be started once");
    }//如果状态不是Ready，就抛出异常，表示只能启动一次

    stop_requested_ = false;
    worker_error_ = nullptr;
    frames_.clear();
    next_sequence_ = 0;
    state_ = CameraSessionState::Running;

    try {
      //工作线程启动，绑定run函数
      worker_ = std::thread(&CameraSession::run, this);
    } catch (...) {
      state_ = CameraSessionState::Stopped;
      throw;//抛出异常
    }
  }//Ready->Running
   //如果有异常，则state_还是置为Stopped，相当于状态回滚
}

void CameraSession::stop() {
  const std::scoped_lock lifecycle_lock{lifecycle_mutex_};
  FrameSource* source = nullptr;//source_指针置空

  {
    const std::scoped_lock lock{mutex_};
    if (state_ == CameraSessionState::Ready) {
      state_ = CameraSessionState::Stopped;
      return;
    }

    stop_requested_ = true;
    if (state_ == CameraSessionState::Running) {
      state_ = CameraSessionState::Stopping;
    }
    source = source_.get();//取出source_指针，后面调用request_stop()，通知后台线程停止读取帧
  }

  
  if (source != nullptr) {
    source->request_stop();
  }
  join_worker();

  const std::scoped_lock lock{mutex_};
  if (state_ == CameraSessionState::Stopping) {
    state_ = CameraSessionState::Stopped;
  }
}//stop()方法是幂等的，调用多次不会有副作用。

std::optional<CapturedFrame> CameraSession::try_take_latest() {
  const std::scoped_lock lock{mutex_};
  if (frames_.empty()) {
    return std::nullopt;
  }

  CapturedFrame latest = std::move(frames_.back());
  frames_.clear();
  return latest;
}//获取最新的捕获帧，如果没有帧可用，则返回std::nullopt。清除缓冲区中的所有帧，只保留最新的一帧。

std::size_t CameraSession::buffered_frame_count() const {
  const std::scoped_lock lock{mutex_};
  return frames_.size();
}//返回缓冲区中捕获帧的数量。

CameraSessionState CameraSession::state() const {
  const std::scoped_lock lock{mutex_};
  return state_;
}//返回当前的摄像头会话状态。

void CameraSession::rethrow_if_error() const {
  std::exception_ptr error;//异常指针
  {
    const std::scoped_lock lock{mutex_};
    error = worker_error_;
  }//锁内获取异常指针，锁外重新抛出异常
  if (error) {
    std::rethrow_exception(error);
  }
}

void CameraSession::join_worker() {
  std::thread worker;
  {
    const std::scoped_lock lock{mutex_};
    if (worker_.joinable()) {
      worker = std::move(worker_);
    }
  }
  if (worker.joinable()) {
    worker.join();
  }
}//检查后台线程是否可连接，如果可连接，则将其移动到局部变量worker中，并在锁外调用join()方法等待线程结束。

void CameraSession::run() {
  using Clock = std::chrono::steady_clock;

  std::size_t frames_in_window = 0;
  double capture_fps = 0.0;
  auto rate_started_at = Clock::now();

  try {
    for (;;) {
      {
        const std::scoped_lock lock{mutex_};
        if (stop_requested_) {
          break;
        }
      }

      // FrameSource::read() can block or enter third-party code. It is never
      // called while the session mutex is held.
      cv::Mat image = source_->read();
      if (image.empty()) {
        continue;
      }

      ++frames_in_window;//帧计数器加1
      const auto now = Clock::now();
      const auto rate_window = now - rate_started_at;
      if (rate_window >= std::chrono::seconds{1}) {
        const auto seconds = std::chrono::duration<double>{rate_window}.count();
        capture_fps = static_cast<double>(frames_in_window) / seconds;//计算帧率
        frames_in_window = 0;
        rate_started_at = now;
      }

      {
        const std::scoped_lock lock{mutex_};
        if (stop_requested_) {
          break;
        }

        ++next_sequence_;//帧序列号加1
        if (frames_.size() == 2) {
          frames_.pop_front();
        }//如果缓冲区中已经有两帧，则移除最旧的一帧，保持缓冲区大小为2
        frames_.push_back(
            {std::move(image), next_sequence_, capture_fps});//更新缓冲区，添加最新捕获的帧，包括图像、序列号和帧率
      }
    }
  } catch (...) {
    const std::scoped_lock lock{mutex_};
    //如果捕获到异常，则设置worker_error_，并将状态设置为Failed
    if (!stop_requested_) {
      worker_error_ = std::current_exception();
      state_ = CameraSessionState::Failed;
      return;
    }
  }

  const std::scoped_lock lock{mutex_};
  if (state_ == CameraSessionState::Running ||
      state_ == CameraSessionState::Stopping) {
    state_ = CameraSessionState::Stopped;
  }//如果状态是Running或Stopping，则将状态设置为Stopped，表示会话已停止。
}

}  // namespace sturdy_guide::camera
