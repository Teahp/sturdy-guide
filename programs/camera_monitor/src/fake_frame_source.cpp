#include "camera/fake_frame_source.hpp"

#include <chrono>
#include <stdexcept>
#include <thread>
#include <utility>

namespace sturdy_guide::camera
{

FakeFrameSource::FakeFrameSource(
    std::vector<cv::Mat> frames,
    std::optional<std::size_t> fail_after)
    : frames_(std::move(frames)),
      fail_after_(fail_after)
{
}

cv::Mat FakeFrameSource::read()
{
    if (stop_requested_.load())
    {
        return {};
    }

    if (fail_after_ && read_count_ >= *fail_after_)
    {
        throw std::runtime_error("Fake frame source failure");
    }

    if (frames_.empty())
    {
        return {};
    }

    cv::Mat frame = frames_[read_count_ % frames_.size()].clone();
    ++read_count_;

    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    return frame;
}

void FakeFrameSource::request_stop() noexcept
{
    stop_requested_.store(true);
}

}