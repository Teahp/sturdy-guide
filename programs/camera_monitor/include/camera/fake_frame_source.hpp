#pragma once
#include "frame_source.hpp"

namespace sturdy_guide::camera {

// 假相机：生成测试画面，不需要真实设备
class FakeFrameSource : public FrameSource {
public:
    explicit FakeFrameSource(int width = 640, int height = 480);
    std::optional<cv::Mat> read() override;
    int width() const override { return width_; }
    int height() const override { return height_; }

private:
    int width_;
    int height_;
    int frame_counter_ = 0;
};

} // namespace
