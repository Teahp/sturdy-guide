#include "camera/camera_session.hpp"
#include <stdexcept>
#include <chrono>

namespace sturdy_guide::camera {

CameraSession::CameraSession(std::unique_ptr<FrameSource> src)
    : source_(std::move(src)) {
    if (!source_) throw std::invalid_argument("相机不能为空");
    frame_buffer_.resize(MAX_FRAMES);
}

CameraSession::~CameraSession() { stop(); }

void CameraSession::start() {
    if (worker_.joinable()) throw std::logic_error("已启动，不能重复");
    running_ = true;
    stop_requested_ = false;
    background_exception_ = nullptr;
    worker_ = std::thread(&CameraSession::workerLoop, this);
}

void CameraSession::stop() {
    if (worker_.joinable()) {
        stop_requested_ = true;
        running_ = false;
        worker_.join();
    }
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& f : frame_buffer_) f.release();
    has_new_frame_ = false;
}

std::optional<cv::Mat> CameraSession::getLatestFrame() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!has_new_frame_ || !running_) return std::nullopt;
    size_t last_index = (next_write_index_ == 0) ? (MAX_FRAMES - 1) : (next_write_index_ - 1);
    return frame_buffer_[last_index].clone();
}

void CameraSession::checkException() {
    if (background_exception_) std::rethrow_exception(background_exception_);
}

int CameraSession::width() const { return source_ ? source_->width() : 0; }
int CameraSession::height() const { return source_ ? source_->height() : 0; }

void CameraSession::workerLoop() {
    try {
        while (running_ && !stop_requested_) {
            auto frame_opt = source_->read();
            if (!frame_opt) {
                throw std::runtime_error("相机读帧失败（设备可能断开）");
            }
            cv::Mat frame = std::move(*frame_opt);
            if (frame.empty()) continue;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                frame_buffer_[next_write_index_] = frame.clone();
                next_write_index_ = (next_write_index_ + 1) % MAX_FRAMES;
                has_new_frame_ = true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    } catch (...) {
        std::lock_guard<std::mutex> lock(mutex_);
        background_exception_ = std::current_exception();
        running_ = false;
    }
}

} // namespace
