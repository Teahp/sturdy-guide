#pragma once
#include "frame_source.h"
#include <atomic>
#include <optional>

class FakeFrameSource : public FrameSource {
public:
    enum class FailureMode {
        None,
        Stop,
        Overflow,
        EmptyFrame,
        ReadException
    };

    explicit FakeFrameSource(FailureMode mode = FailureMode::None);
    bool open() override;
    std::optional<cv::Mat> read() override;
    void close() override;
    bool isOpen() const override;
    void setFailureMode(FailureMode mode);

private:
    std::atomic<bool> opened_{false};
    std::atomic<FailureMode> mode_{FailureMode::None};
    int frame_counter_ = 0;
};
