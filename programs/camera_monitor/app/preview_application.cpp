#include "preview_application.h"
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace camera {

PreviewApplication::PreviewApplication(std::unique_ptr<CameraSession> session)
    : session_(std::move(session)) {
    if (!session_) {
        throw std::invalid_argument("PreviewApplication requires a valid session");
    }
}

PreviewApplication::~PreviewApplication() {
    if (window_created_) {
        cv::destroyWindow(window_name_);
    }
}

int PreviewApplication::run() {
    try {
        // 创建窗口
        cv::namedWindow(window_name_, cv::WINDOW_NORMAL);
        window_created_ = true;

        // 启动采集
        session_->start();

        // FPS 统计
        auto rate_started_at = std::chrono::steady_clock::now();
        std::size_t frames_in_window = 0;

        while (!quit_requested_) {
            // 检查后台错误
            if (session_->hasError()) {
                throw std::runtime_error(session_->getErrorMessage());
            }

            // 获取最新帧
            Frame frame;
            if (session_->tryGetLatestFrame(frame)) {
                ++displayed_frames_;
                ++frames_in_window;

                // 计算 FPS
                const auto now = std::chrono::steady_clock::now();
                const auto rate_window = now - rate_started_at;
                if (rate_window >= std::chrono::seconds{1}) {
                    const auto seconds = std::chrono::duration<double>{rate_window}.count();
                    current_fps_ = static_cast<double>(frames_in_window) / seconds;
                    frames_in_window = 0;
                    rate_started_at = now;
                }

                // 绘制叠加信息
                drawOverlay(frame, current_fps_);

                // 显示（在锁外执行）
                cv::imshow(window_name_, frame.image);

                // 处理按键
                const int key = cv::waitKey(1) & 0xFF;
                handleKey(key, frame);
            } else {
                // 没有新帧，短暂等待
                cv::waitKey(1);
            }

            // 检查窗口是否关闭
            const double visibility = cv::getWindowProperty(
                window_name_, cv::WND_PROP_VISIBLE);
            if (visibility >= 0.0 && visibility < 1.0) {
                quit_requested_ = true;
            }
        }

        // 正常退出
        session_->stop();
        cv::destroyWindow(window_name_);
        window_created_ = false;
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "camera: " << e.what() << '\n';
        session_->stop();
        if (window_created_) {
            cv::destroyWindow(window_name_);
            window_created_ = false;
        }
        return 1;
    }
}

void PreviewApplication::drawOverlay(Frame& frame, double fps) {
    std::ostringstream overlay;
    overlay << "frame " << frame.sequence << "  " << std::fixed
            << std::setprecision(1) << fps << " FPS";
    cv::putText(frame.image, overlay.str(), cv::Point{20, 36},
                cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar{40, 230, 90}, 2,
                cv::LINE_AA);
}

bool PreviewApplication::saveScreenshot(const Frame& frame) {
    std::filesystem::create_directories("captures");
    std::filesystem::path filename;
    do {
        ++saved_frames_;
        filename = std::filesystem::path{"captures"} /
                   ("capture-" + std::to_string(saved_frames_) + ".png");
    } while (std::filesystem::exists(filename));

    if (!cv::imwrite(filename.string(), frame.image)) {
        throw std::runtime_error("failed to save " + filename.string());
    }
    std::cout << "Saved " << filename << '\n';
    return true;
}

void PreviewApplication::handleKey(int key, const Frame& frame) {
    if (key == 'q' || key == 'Q') {
        quit_requested_ = true;
    } else if (key == 's' || key == 'S') {
        saveScreenshot(frame);
    }
}

}  // namespace camera
