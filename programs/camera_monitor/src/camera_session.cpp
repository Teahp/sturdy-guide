#include "camera_session.h"
#include <chrono>
#include <iostream>

CameraSession::CameraSession(std::unique_ptr<FrameSource> source)
    : source_(std::move(source)) {}

CameraSession::~CameraSession() {
    stop();
    join();
}

void CameraSession::start() {
    std::lock_guard<std::mutex> lock(mtx_);
    if (running_) return;
    if (!source_ || !source_->isOpen()) {
        throw std::runtime_error("FrameSource not open or invalid");
    }
    running_ = true;
    stop_requested_ = false;
    last_exception_ = nullptr;
    worker_ = std::thread(&CameraSession::workerLoop, this);
}

void CameraSession::stop() {
    {
        std::lock_guard<std::mutex> lock(mtx_);
        if (!running_) return;
        stop_requested_ = true;
    }
    cv_.notify_one();
}

void CameraSession::join() {
    if (worker_.joinable()) {
        worker_.join();
    }
}

bool CameraSession::isRunning() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return running_;
}

void CameraSession::workerLoop() {
    while (true) {
        {
            std::lock_guard<std::mutex> lock(mtx_);
            if (stop_requested_) break;
        }

        std::optional<cv::Mat> frame;
        try {
            frame = source_->read();
        } catch (const std::exception& e) {
            std::lock_guard<std::mutex> lock(mtx_);
            last_exception_ = std::make_exception_ptr(e);
            stop_requested_ = true;
            break;
        }

        {
            std::lock_guard<std::mutex> lock(mtx_);
            if (stop_requested_) break;

            if (frame.has_value()) {
                if (buffer_.size() >= MAX_BUFFER) {
                    buffer_.pop();
                }
                buffer_.push(std::move(frame.value()));
                cv_.notify_one();
            } else {
                last_exception_ = std::make_exception_ptr(
                    std::runtime_error("Read empty frame")
                );
                stop_requested_ = true;
                break;
            }
        }
    }

    if (source_) source_->close();
    {
        std::lock_guard<std::mutex> lock(mtx_);
        running_ = false;
    }
}

std::optional<cv::Mat> CameraSession::getLatestFrame() {
    std::lock_guard<std::mutex> lock(mtx_);
    if (buffer_.empty()) return std::nullopt;
    cv::Mat frame = buffer_.back();
    std::queue<cv::Mat> empty;
    std::swap(buffer_, empty);
    return frame;
}

std::exception_ptr CameraSession::getException() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return last_exception_;
}
