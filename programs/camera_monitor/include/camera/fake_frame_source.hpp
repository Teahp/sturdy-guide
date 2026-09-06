#pragma once

#include "camera/frame_source.hpp"

#include <atomic>
#include <cstddef>
#include <optional>
#include <vector>

namespace sturdy_guide::camera
{

class FakeFrameSource final : public FrameSource
{
public:
    explicit FakeFrameSource(
        std::vector<cv::Mat> frames,
        std::optional<std::size_t> fail_after = std::nullopt);

    cv::Mat read() override;
    void request_stop() noexcept override;

private:
    std::vector<cv::Mat> frames_;
    std::optional<std::size_t> fail_after_;
    std::atomic<bool> stop_requested_{false};
    std::size_t read_count_{0};
};

}