#include "camera/camera_session.hpp"

#include <stdexcept>
#include <utility>

namespace sturdy_guide::camera {

CameraSession::CameraSession(std::unique_ptr<FrameSource> source)
    : source_{std::move(source)} {}

CameraSession::~CameraSession() {
  // 先请求停止，再 join，保证工作线程不会访问已经析构的成员。
  stop();
  try {
    wait();
  } catch (...) {
    // 析构函数不能继续抛出后台错误；显式 wait() 才是观察错误的边界。
  }
}

void CameraSession::start() {
  std::thread completed_worker;
  {
    std::scoped_lock lock{mutex_};
    if (running_) {
      throw std::logic_error("the camera session is already running");
    }

    // 上一次自然结束的线程仍需 join，先移出临界区再 join，避免持锁阻塞。
    if (worker_.joinable()) {
      completed_worker = std::move(worker_);
    }
    stop_requested_ = false;
    error_ = nullptr;
    running_ = true;
  }

  if (completed_worker.joinable()) {
    completed_worker.join();
  }

  try {
    std::scoped_lock lock{mutex_};
    worker_ = std::thread{&CameraSession::run, this};
  } catch (...) {
    std::scoped_lock lock{mutex_};
    running_ = false;
    throw;
  }
}

void CameraSession::stop() {
  {
    const std::scoped_lock lock{mutex_};
    stop_requested_ = true;
  }
  // 唤醒任何基于 state_changed_ 的等待路径；重复调用是幂等的。
  state_changed_.notify_all();
}

bool CameraSession::is_running() const {
  const std::scoped_lock lock{mutex_};
  return running_;
}

bool CameraSession::try_latest_frame(cv::Mat& image,
                                     std::size_t& frame_number) {
  const std::scoped_lock lock{mutex_};
  if (frames_.empty()) {
    return false;
  }

  // 消费者只关心最新帧：取队尾并丢弃其余旧帧。
  const Frame& latest = frames_.back();
  image = latest.image;
  frame_number = latest.index;
  frames_.clear();
  return true;
}

void CameraSession::wait() {
  std::thread worker;
  {
    std::scoped_lock lock{mutex_};
    if (worker_.joinable()) {
      worker = std::move(worker_);
    }
  }
  if (worker.joinable()) {
    worker.join();
  }

  std::exception_ptr error;
  {
    const std::scoped_lock lock{mutex_};
    error = std::exchange(error_, nullptr);
  }
  if (error) {
    std::rethrow_exception(error);
  }
}

void CameraSession::run() {
  std::size_t captured = 0;
  try {
    for (;;) {
      {
        const std::scoped_lock lock{mutex_};
        if (stop_requested_) {
          break;
        }
      }

      // 摄像头读取是慢速 I/O，必须在临界区之外执行，避免阻塞 stop()。
      cv::Mat image = source_->read();
      if (image.empty()) {
        // 来源不再产生有效帧：不发布空帧，按正常结束退出。
        break;
      }

      ++captured;
      {
        std::scoped_lock lock{mutex_};
        // 有界缓冲：最多保留两帧，已满时丢弃最旧的一帧。
        if (frames_.size() >= 2) {
          frames_.pop_front();
        }
        frames_.push_back(Frame{captured, std::move(image)});
      }
      state_changed_.notify_all();
    }
  } catch (...) {
    const std::scoped_lock lock{mutex_};
    error_ = std::current_exception();
  }

  {
    const std::scoped_lock lock{mutex_};
    running_ = false;
  }
  state_changed_.notify_all();
}

}  // namespace sturdy_guide::camera
