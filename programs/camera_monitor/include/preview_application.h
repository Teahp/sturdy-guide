#pragma once
#include <memory>
#include <string>
#include <chrono>
#include "camera_session.h"

class PreviewApplication {
public:
    explicit PreviewApplication(std::unique_ptr<CameraSession> session);
    int run(int argc, char** argv);

private:
    bool handleKey(char key);
    void saveScreenshot(const cv::Mat& frame);
    void showError(const std::string& msg);

    std::unique_ptr<CameraSession> session_;
    std::string window_name_ = "Camera Monitor";
    int frame_count_ = 0;
    double fps_ = 0.0;
    std::chrono::steady_clock::time_point last_fps_time_;
};
