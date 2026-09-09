#include "camera/camera_session.h"
#include <chrono>
#include <iostream>
#include <stdexcept>

namespace camera {

CameraSession::CameraSession(std::unique_ptr<FrameSource> source)
    : source_(std::move(source)) {
    if (!source_) {
        throw std::invalid_argument("CameraSession requires a valid FrameSource");
    }
}

CameraSession::~CameraSession() {
    stop();  // 确保线程被 join
}

void CameraSession::start() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (running_) {
        return;  // 已经运行，忽略重复调用
    }

    // 重置状态
    stop_requested_ = false;
    worker_exception_ = nullptr;
    error_message_.clear();
    has_new_frame_ = false;
    write_index_ = 0;
    frame_sequence_ = 0;

    // 启动工作线程
    worker_ = std::thread(&CameraSession::workerLoop, this);
    running_ = true;
}

void CameraSession::stop() {
    // 先检查是否需要停止（避免不必要的锁）
    if (!running_.load(std::memory_order_acquire)) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!running_) {
            return;  // 另一个线程已经停止了
        }
        stop_requested_ = true;
    }

    // 唤醒工作线程（虽然当前实现中工作线程不需要等待）
    cv_.notify_all();

    // 等待线程结束（在锁外 join，避免死锁）
    if (worker_.joinable()) {
        worker_.join();
    }

    // 更新状态
    {
        std::lock_guard<std::mutex> lock(mutex_);
        running_ = false;
    }
}

bool CameraSession::tryGetLatestFrame(Frame& frame) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!has_new_frame_) {
        return false;
    }

    // 双缓冲策略：
    // - 工作线程写入 buffers_[write_index_]
    // - 主线程读取 buffers_[write_index_]
    // - 如果主线程消费慢，工作线程会覆盖另一个缓冲区（最旧的帧）
    // - 这样始终保留最新的帧，丢弃旧帧
    frame = buffers_[write_index_];
    has_new_frame_ = false;  // 消费掉新帧标记

    return true;
}

bool CameraSession::hasError() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return worker_exception_ != nullptr;
}

std::string CameraSession::getErrorMessage() const {
    std::lock_guard<std::mutex> lock(mutex_);

    if (worker_exception_) {
        try {
            std::rethrow_exception(worker_exception_);
        } catch (const std::exception& e) {
            return e.what();
        } catch (...) {
            return "unknown worker thread error";
        }
    }

    return error_message_;
}

bool CameraSession::isRunning() const {
    return running_.load(std::memory_order_acquire);
}

void CameraSession::workerLoop() {
    try {
        // 打开设备（在工作线程中执行，避免阻塞主线程）
        source_->open();

        while (!stop_requested_.load(std::memory_order_acquire)) {
            Frame frame;

            // 在锁外执行 I/O 操作（摄像头读取可能阻塞）
            bool read_success = false;
            try {
                read_success = source_->read(frame);
            } catch (const std::exception& e) {
                throw std::runtime_error("read failed: " + std::string(e.what()));
            }

            if (!read_success) {
                // 读取失败，抛出异常
                throw std::runtime_error("camera stopped returning valid frames");
            }

            if (frame.isEmpty()) {
                // 空帧不发布，继续下一轮
                continue;
            }

            // 在锁内更新缓冲区和帧序号（临界区很小，只包含内存操作）
            {
                std::lock_guard<std::mutex> lock(mutex_);

                // 分配帧序号（在锁内，保护 frame_sequence_）
                frame.sequence = frame_sequence_++;

                // 双缓冲策略：
                // 始终写入另一个缓冲区（不是主线程可能正在读取的那个）
                // 如果主线程消费慢，会覆盖最旧的帧
                write_index_ = (write_index_ + 1) % buffers_.size();
                buffers_[write_index_] = std::move(frame);
                has_new_frame_ = true;
            }

            cv_.notify_all();  // 通知等待者（如果有）
        }

        // 正常退出，关闭设备
        source_->close();

    } catch (...) {
        // 捕获所有异常，保存到 exception_ptr
        // 注意：不要在这里修改 running_，让 stop() 来处理
        std::lock_guard<std::mutex> lock(mutex_);
        worker_exception_ = std::current_exception();
        error_message_ = "worker thread error";

        // 尝试关闭设备
        try {
            source_->close();
        } catch (...) {
            // 忽略关闭时的异常
        }
    }
}

}  // namespace camera
