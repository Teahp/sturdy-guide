#pragma once
#include "camera/frame_source.hpp"
#include <cstddef>
#include <chrono>
#include <opencv2/core.hpp>

namespace camera {

// 模拟帧源：用于自动化测试，生成带序号的测试帧，可模拟失败
class FakeFrameSource : public FrameSource {
public:
    FakeFrameSource(int width, int height, double fps = 30.0);

    // 设置第N帧后抛出读取异常，0表示不失败
    void set_fail_after(size_t fail_after);

    void read(cv::Mat& frame) override;

    // 获取已生产的总帧数
    size_t frame_count() const;

private:
    int width_;
    int height_;
    double fps_;
    std::chrono::steady_clock::time_point next_frame_time_;
    size_t frame_count_ = 0;
    size_t fail_after_ = 0;
};

} // namespace camera
