//
// Created by xzhou on 2026/9/7.
//
// programs/camera_monitor/src/camera_session.cpp
#include "camera/camera_session.hpp"
#include <chrono>
#include <iostream>

namespace camera {

CameraSession::CameraSession(std::unique_ptr<FrameSource> source)
    : source_(std::move(source)) {}

CameraSession::~CameraSession() {
    stop();
}

void CameraSession::start() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (state_ == State::Running) {
        return; // 已经运行
    }

    if (worker_.joinable()) {
        worker_.join();
    }

    state_ = State::Running;
    stop_requested_ = false;
    last_error_ = std::nullopt;

    worker_ = std::thread(&CameraSession::workerLoop, this);
}

void CameraSession::stop() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (state_ == State::Stopped || state_ == State::Idle) {
            return; // 已经停止
        }
        stop_requested_ = true;
        state_ = State::Stopping;
        cv_.notify_all();
    }

    if (worker_.joinable()) {
        worker_.join();
    }

    std::lock_guard<std::mutex> lock(mutex_);
    state_ = State::Stopped;
}

std::optional<FrameResult> CameraSession::tryGetLatestFrame() {
    std::lock_guard<std::mutex> lock(mutex_);
    return latest_frame_;
}

CameraSession::State CameraSession::getState() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return state_;
}

std::optional<std::string> CameraSession::getLastError() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return last_error_;
}

void CameraSession::workerLoop() {
    while (true) {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            if (stop_requested_) {
                break;
            }
        }

        // 读取帧（在锁外执行）
        auto result = source_->read();

        {
            std::lock_guard<std::mutex> lock(mutex_);

            if (stop_requested_) {
                break;
            }

            if (result.has_value()) {
                latest_frame_ = result;
            } else {
                // 读取失败
                last_error_ = "Camera read failed";
                // 继续尝试，不退出
            }
        }

        // 让出 CPU，避免忙等
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

} // namespace camera// programs/camera_monitor/src/camera_session.cpp
#include "camera/camera_session.hpp"
#include <chrono>
#include <iostream>

namespace camera {

CameraSession::CameraSession(std::unique_ptr<FrameSource> source)
    : source_(std::move(source)) {}

CameraSession::~CameraSession() {
    stop();
}

void CameraSession::start() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (state_ == State::Running) {
        return; // 已经运行
    }

    if (worker_.joinable()) {
        worker_.join();
    }

    state_ = State::Running;
    stop_requested_ = false;
    last_error_ = std::nullopt;

    worker_ = std::thread(&CameraSession::workerLoop, this);
}

void CameraSession::stop() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (state_ == State::Stopped || state_ == State::Idle) {
            return; // 已经停止
        }
        stop_requested_ = true;
        state_ = State::Stopping;
        cv_.notify_all();
    }

    if (worker_.joinable()) {
        worker_.join();
    }

    std::lock_guard<std::mutex> lock(mutex_);
    state_ = State::Stopped;
}

std::optional<FrameResult> CameraSession::tryGetLatestFrame() {
    std::lock_guard<std::mutex> lock(mutex_);
    return latest_frame_;
}

CameraSession::State CameraSession::getState() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return state_;
}

std::optional<std::string> CameraSession::getLastError() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return last_error_;
}

void CameraSession::workerLoop() {
    while (true) {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            if (stop_requested_) {
                break;
            }
        }

        // 读取帧（在锁外执行）
        auto result = source_->read();

        {
            std::lock_guard<std::mutex> lock(mutex_);

            if (stop_requested_) {
                break;
            }

            if (result.has_value()) {
                latest_frame_ = result;
            } else {
                // 读取失败
                last_error_ = "Camera read failed";
                // 继续尝试，不退出
            }
        }

        // 让出 CPU，避免忙等
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

} // namespace camera