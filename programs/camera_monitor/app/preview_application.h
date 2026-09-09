#pragma once

#include "camera/camera_session.h"
#include <opencv2/core.hpp>
#include <cstdint>
#include <memory>
#include <string>

namespace camera {

// PreviewApplication 负责 UI 相关操作
// 在主线程运行：imshow、waitKey、截图
// 不直接访问 CameraSession 的 mutex 或工作线程
class PreviewApplication {
public:
    explicit PreviewApplication(std::unique_ptr<CameraSession> session);
    ~PreviewApplication();

    // 运行主循环，返回退出码（0 表示正常退出）
    int run();

private:
    void drawOverlay(Frame& frame, double fps);
    bool saveScreenshot(const Frame& frame);
    void handleKey(int key, const Frame& frame);

    std::unique_ptr<CameraSession> session_;
    std::string window_name_{"Sturdy Guide Camera"};

    std::uint64_t displayed_frames_{0};
    std::uint64_t saved_frames_{0};
    double current_fps_{0.0};

    bool quit_requested_{false};
    bool window_created_{false};
};

}  // namespace camera
