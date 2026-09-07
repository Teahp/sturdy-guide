#pragma once
#include "camera_session.hpp"

namespace sturdy_guide::camera {

// 主线程窗口应用，负责显示、按键和截图
class PreviewApplication {
public:
    explicit PreviewApplication(CameraSession& session);
    ~PreviewApplication() = default;
    void run();

private:
    CameraSession& session_;
    int screenshot_counter_ = 0;
};

} // namespace
