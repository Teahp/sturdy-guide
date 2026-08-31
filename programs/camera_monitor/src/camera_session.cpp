#include "camera/camera_session.hpp"
#include <stdexcept>

namespace camera {

CameraSession::CameraSession(std::unique_ptr<FrameSource> source)
    : source_(std::move(source)) {}

CameraSession::~CameraSession() {
    stop();
    join();
}

void CameraSession::start() {
    // 使用unique_lock支持手动解锁，避免join旧线程时死锁
    std::unique_lock<std::mutex> lock(mutex_);

    if (running_.load()) {
        return; // 已运行则直接返回，幂等
    }

    if (!thread_joined_) {
        // 先解锁，再回收旧线程，否则会死锁
        lock.unlock();
        worker_.join();
        lock.lock();
        thread_joined_ = true;
    }

    // 重置状态
    running_.store(true);
    thread_joined_ = false;
    error_ = nullptr;
    frame_buffer_.clear();

    worker_ = std::thread(&CameraSession::worker_loop, this);
}

void CameraSession::stop() {
    running_.store(false);
    cv_.notify_all();
}

void CameraSession::join() {
    std::unique_lock<std::mutex> lock(mutex_);
    
    if (!worker_.joinable()) {
        return;
    }

    // 必须先解锁再join！否则后台线程拿不到锁无法退出，造成死锁
    lock.unlock();
    worker_.join();
    
    lock.lock();
    thread_joined_ = true;
}

bool CameraSession::try_get_latest_frame(Frame& out_frame) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (error_) {
        std::rethrow_exception(error_);
    }

    if (frame_buffer_.empty()) {
        return false;
    }

    // 取最新帧（队尾），丢弃所有旧帧
    out_frame = frame_buffer_.back();
    out_frame.image = out_frame.image.clone();
    frame_buffer_.clear();

    return true;
}

bool CameraSession::has_error() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return error_ != nullptr;
}

std::exception_ptr CameraSession::get_error() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return error_;
}

void CameraSession::worker_loop() {
    size_t next_id = 0;

    try {
        while (running_.load()) {
            cv::Mat image;
            // 读帧在临界区外执行：避免持锁做慢速IO
            source_->read(image);

            if (image.empty()) {
                throw std::runtime_error("received empty frame");
            }

            {
                std::lock_guard<std::mutex> lock(mutex_);
                // 缓冲区满则丢弃最旧帧，保证始终存最新的2帧
                if (frame_buffer_.size() >= 2) {
                    frame_buffer_.pop_front();
                }
                frame_buffer_.push_back(Frame{next_id, image.clone()});
            }

            ++next_id;
        }
    } catch (...) {
        std::lock_guard<std::mutex> lock(mutex_);
        error_ = std::current_exception();
        running_.store(false);
    }

    running_.store(false);
}

} // namespace camera
