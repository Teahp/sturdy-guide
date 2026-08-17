#pragma once

#include "frame_source.hpp"
#include <opencv2/core.hpp>
#include <atomic>

namespace camera {

class FakeFrameSource : public FrameSource {
public:
    FakeFrameSource(int width = 640, int height = 480, int fps = 30);
    ~FakeFrameSource() override = default;

    std::optional<Frame> read() override;
    bool isOpened() const override { return true; }
    double getProperty(int propId) const override;
    bool setProperty(int propId, double value) override;

    void setShouldFail(bool fail) { shouldFail_ = fail; }

private:
    int width_, height_, fps_;
    int64_t frame_counter_ = 0;
    std::atomic<bool> shouldFail_{false};
    double timestamp_base_;
};

} // namespace camera
