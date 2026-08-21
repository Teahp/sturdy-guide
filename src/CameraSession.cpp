#include "../include/CameraSession.hpp"
#include <chrono>
#include <iostream>

CameraSession::CameraSession(std::unique_ptr<FrameSource> source)
    : source_(std::move(source)) {}

CameraSession::~CameraSession() {
    stop();
    join();
}

void CameraSession::start() {
    if (running_.exchange(true)) {
        return;
    }
    worker_ = std::thread(&CameraSession::acquisitionLoop, this);
}

void CameraSession::stop() {
    running_ = false;
}

void CameraSession::join() {
    if (worker_.joinable()) {
        worker_.join();
    }
}

std::optional<cv::Mat> CameraSession::getLatestFrame() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (frameQueue_.empty()) {
        return std::nullopt;
    }
    cv::Mat frame = frameQueue_.back();
    return frame;
}

void CameraSession::acquisitionLoop() {
    while (running_) {
        auto frame = source_->getFrame();
        if (frame) {
            std::lock_guard<std::mutex> lock(mutex_);
            if (frameQueue_.size() >= MAX_QUEUE_SIZE) {
                frameQueue_.pop();
            }
            frameQueue_.push(*frame);
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}
