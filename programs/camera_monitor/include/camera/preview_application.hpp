#pragma once

#include "camera/camera_session.hpp"

#include <chrono>
#include <memory>
#include <string>

namespace sturdy_guide::camera
{

class PreviewApplication
{
public:
    explicit PreviewApplication(std::unique_ptr<FrameSource> source);

    int run();

private:
    void update_fps(const Frame& frame);
    void draw_overlay(cv::Mat& image, const Frame& frame) const;
    void save_screenshot(const cv::Mat& image);

    CameraSession session_;
    double fps_{0.0};
    std::chrono::steady_clock::time_point previous_frame_time_{};
    std::size_t screenshot_number_{0};
    std::string window_name_{"Sturdy Guide Camera"};
};

}