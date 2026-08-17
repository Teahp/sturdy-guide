#include "camera/session.hpp"
#include <chrono>
#include <algorithm>

namespace camera {

CameraSession::CameraSession(std::unique_ptr<FrameSource> source)
    : source_(std::move(source)) {
    if (!source_) {
        throw std::invalid_argument("FrameSource is null");
    }
}

CameraSession::~CameraSession() {
    stop();
}

void CameraSession::start() {
    if (running_.load()) {
        return;
    }
    if (worker_.joinable()) {
        worker_.join();
    }
    shouldStop_.store(false);
    running_.store(true);
    worker_ = std::thread(&CameraSession::workerLoop, this);
}

void CameraSession::stop() {
    if (!running_.load()) {
        return;
    }
    shouldStop_.store(true);
    {
        std::lock_guard<std::mutex> lock(mutex_);
        cv_.notify_all();
    }
    if (worker_.joinable()) {
        worker_.join();
    }
    running_.store(false);
}

bool CameraSession::isRunning() const {
    return running_.load();
}

bool CameraSession::getLatestFrame(Frame& outFrame) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (buffer_.empty()) {
        return false;
    }
    outFrame = buffer_.back();
    return true;
}

bool CameraSession::hasException() const {
    std::lock_guard<std::mutex> lock(exceptionMutex_);
    return exceptionPtr_ != nullptr;
}

void CameraSession::rethrowException() {
    std::lock_guard<std::mutex> lock(exceptionMutex_);
    if (exceptionPtr_) {
        std::rethrow_exception(exceptionPtr_);
    }
}

void CameraSession::workerLoop() {
    try {
        while (!shouldStop_.load()) {
            auto frameOpt = source_->read();
            if (!frameOpt.has_value()) {
                shouldStop_.store(true);
                break;
            }
            {
                std::lock_guard<std::mutex> lock(mutex_);
                if (buffer_.size() >= 2) {
                    buffer_.erase(buffer_.begin());
                }
                buffer_.push_back(std::move(frameOpt.value()));
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    } catch (...) {
        {
            std::lock_guard<std::mutex> lock(exceptionMutex_);
            exceptionPtr_ = std::current_exception();
        }
        running_.store(false);
        shouldStop_.store(true);
    }
    running_.store(false);
}

} // namespace camera
