#include "camera/camera_session.hpp"
#include <chrono>
#include <stdexcept>
#include <utility>

namespace sturdy_guide::camera
{

CameraSession::CameraSession(std::unique_ptr<FrameSource> source)
    : source_(std::move(source))
{
    if (!source_)
    {
        throw std::invalid_argument("FrameSource must not be null");
    }
}

CameraSession::~CameraSession()
{
    stop();
}

void CameraSession::start()
{
    std::lock_guard lock(mutex_);

    if (state_ != SessionState::stopped || worker_.joinable())
    {
        throw std::logic_error("CameraSession is already running");
    }

    frames_.clear();
    background_error_ = nullptr;
    state_ = SessionState::running;
    worker_ = std::thread(&CameraSession::worker_loop, this);
}

void CameraSession::stop() noexcept
{
    {
        std::lock_guard lock(mutex_);

        if (state_ == SessionState::stopped && !worker_.joinable())
        {
            return;
        }

        if (state_ != SessionState::failed)
        {
            state_ = SessionState::stopping;
        }
    }

    source_->request_stop();
    condition_.notify_all();

    if (worker_.joinable())
    {
        worker_.join();
    }

    std::lock_guard lock(mutex_);
    state_ = SessionState::stopped;
}

std::optional<Frame> CameraSession::latest_frame()
{
    std::lock_guard lock(mutex_);

    if (frames_.empty())
    {
        return std::nullopt;
    }

    Frame frame = std::move(frames_.back());

    if (frames_.size() > 1)
    {
        dropped_frames_ += frames_.size() - 1;
    }

    frames_.clear();
    return frame;
}

SessionState CameraSession::state() const
{
    std::lock_guard lock(mutex_);
    return state_;
}

std::size_t CameraSession::buffered_frame_count() const
{
    std::lock_guard lock(mutex_);
    return frames_.size();
}

std::size_t CameraSession::dropped_frame_count() const
{
    std::lock_guard lock(mutex_);
    return dropped_frames_;
}

void CameraSession::rethrow_background_error() const
{
    std::exception_ptr error;

    {
        std::lock_guard lock(mutex_);
        error = background_error_;
    }

    if (error)
    {
        std::rethrow_exception(error);
    }
}

void CameraSession::worker_loop()
{
    try
    {
        while (true)
        {
            {
                std::lock_guard lock(mutex_);

                if (state_ != SessionState::running)
                {
                    break;
                }
            }

            cv::Mat image = source_->read();

            if (image.empty())
            {
                throw std::runtime_error(
                    "Frame source returned an empty frame");
            }

            Frame frame;
            frame.image = std::move(image);
            frame.sequence = next_sequence_++;
            frame.captured_at = std::chrono::steady_clock::now();

            {
                std::lock_guard lock(mutex_);

                if (state_ != SessionState::running)
                {
                    break;
                }

                if (frames_.size() == 2)
                {
                    frames_.pop_front();
                    ++dropped_frames_;
                }

                frames_.push_back(std::move(frame));
            }

            condition_.notify_one();
        }
    }
    catch (...)
    {
        std::lock_guard lock(mutex_);
        background_error_ = std::current_exception();
        state_ = SessionState::failed;
        condition_.notify_all();
    }
}

}
