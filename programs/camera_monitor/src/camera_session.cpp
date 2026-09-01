#include "camera/camera_session.hpp"

#include <stdexcept>
#include <utility>

CameraSession::CameraSession(std::unique_ptr<FrameSource> source)
    : source_(std::move(source)){}

CameraSession::~CameraSession(){
    stop();
}

void CameraSession::start(){
    std::unique_lock lock{mutex_};
    if (state_ != SessionState::kIdle){
        throw std::logic_error("start: session is not idle");
    }
    stop_requested_ = false;
    error_ = nullptr;
    state_ = SessionState::kRunning;
    newest_.reset();
    oldest_.reset();
    worker_ = std::thread(&CameraSession::worker_loop, this);
}

void CameraSession::stop(){
    {
        std::unique_lock lock{mutex_};
        if (state_ == SessionState::kIdle){
            return;
        }
        stop_requested_ = true;
    }
    wake_.notify_all();
    if (worker_.joinable()){
        worker_.join();
    }
    std::unique_lock lock{mutex_};
    state_ = SessionState::kIdle;
}

std::optional<Frame> CameraSession::consume_latest(){
    std::unique_lock lock{mutex_};
    if (!newest_){
        return std::nullopt;
    }
    Frame result = std::move(*newest_);
    newest_.reset();
    oldest_.reset();
    return result;
}

SessionState CameraSession::state() const{
    std::unique_lock lock{mutex_};
    return state_;
}

std::exception_ptr CameraSession::error()const{
    std::unique_lock lock{mutex_};
    return error_;
}

void CameraSession::worker_loop(){
    try{
        while(true){
            Frame frame = source_->read();
            if (frame.image.empty()){
                throw std::runtime_error("frame source returned an empty frame");
            }

            std::unique_lock lock{mutex_};
            if (stop_requested_){
                return;
            }
            oldest_ = std::move(newest_);
            newest_ = std::move(frame);
            wake_.notify_all();
        }
    }catch(...){
        std::unique_lock lock{mutex_};
        error_ = std::current_exception();
        state_ = SessionState::kFailed;
        stop_requested_ = true;
        wake_.notify_all();
    }
}