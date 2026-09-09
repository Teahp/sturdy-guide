#pragma once

#include "camera/frame_source.h"
#include <opencv2/core.hpp>
#include <atomic>
#include <chrono>
#include <stdexcept>
#include <thread>

namespace camera::testing {

// 测试用假设备：产生确定的假帧或模拟失败
class FakeFrameSource : public FrameSource {
public:
    explicit FakeFrameSource(int fail_after = -1, int frame_delay_ms = 1)
        : fail_after_(fail_after), frame_delay_ms_(frame_delay_ms) {}

    void open() override {
        if (fail_after_ == 0) {
            throw std::runtime_error("simulated open failure");
        }
        opened_ = true;
    }

    bool read(Frame& frame) override {
        if (!opened_) {
            return false;
        }

        if (frame_delay_ms_ > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(frame_delay_ms_));
        }

        const int current = read_count_++;
        if (fail_after_ > 0 && current >= fail_after_) {
            throw std::runtime_error("simulated read failure at frame " +
                                     std::to_string(current));
        }

        // 模拟空帧（每隔一定帧数返回一次空帧）
        if (return_empty_every_ > 0 && current % return_empty_every_ == 0) {
            frame.valid = false;
            frame.image = cv::Mat();
            return true;  // 返回 true 但 frame 是空的
        }

        // 产生一个确定的假帧（100x100 灰度图，像素值等于序列号）
        cv::Mat image(100, 100, CV_8UC1, cv::Scalar(current % 256));
        frame.image = image.clone();
        frame.sequence = current;
        frame.timestamp = std::chrono::steady_clock::now();
        frame.valid = true;
        return true;
    }

    void close() override {
        opened_ = false;
    }

    bool isOpened() const override {
        return opened_;
    }

    std::string description() const override {
        return "FakeFrameSource for testing";
    }

    // 设置每隔多少帧返回一次空帧（0 表示不返回空帧）
    void setReturnEmptyEvery(int every) { return_empty_every_ = every; }

    // 获取已读取的帧数
    int getReadCount() const { return read_count_.load(); }

private:
    int fail_after_;
    int frame_delay_ms_;
    std::atomic<int> read_count_{0};
    std::atomic<bool> opened_{false};
    std::atomic<int> return_empty_every_{0};
};

}  // namespace camera::testing
