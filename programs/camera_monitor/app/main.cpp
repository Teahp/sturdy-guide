//
// Created by xzhou on 2026/9/7.
//
// programs/camera_monitor/include/camera/frame_source.hpp
// programs/camera_monitor/app/main.cpp
#include "camera/frame_source.hpp"
#include "camera/camera_session.hpp"
#include <opencv2/opencv.hpp>
#include <iostream>
#include <string>
#include <chrono>
#include <iomanip>
#include <filesystem>

using namespace camera;

// 参数解析
struct Config {
    int device = 0;
    int width = 1280;
    int height = 720;
};

Config parseArgs(int argc, const char* argv[]) {
    Config config;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: " << argv[0]
                      << " [--device INDEX] [--width PIXELS] [--height PIXELS]\n"
                      << "Keys: S saves a frame; Q exits.\n";
            exit(0);
        }
        if (arg == "--device" && i + 1 < argc) {
            config.device = std::stoi(argv[++i]);
        } else if (arg == "--width" && i + 1 < argc) {
            config.width = std::stoi(argv[++i]);
        } else if (arg == "--height" && i + 1 < argc) {
            config.height = std::stoi(argv[++i]);
        }
    }
    return config;
}

int main(int argc, const char* argv[]) {
    try {
        // 1. 解析参数
        auto config = parseArgs(argc, argv);

        // 2. 创建摄像头源
        auto source = create_real_camera(config.device, config.width, config.height);
        if (!source->isOpened()) {
            throw std::runtime_error("Cannot open camera");
        }
        std::cout << "Opened: " << source->description() << std::endl;

        // 3. 创建会话（启动后台线程）
        CameraSession session(std::move(source));
        session.start();
        std::cout << "Camera session started" << std::endl;

        // 4. 创建窗口
        const std::string window_name = "Sturdy Guide Camera";
        cv::namedWindow(window_name, cv::WINDOW_NORMAL);

        // 5. 主循环（只负责显示和UI）
        std::size_t frame_number = 0;
        std::size_t capture_number = 0;
        auto fps_start = std::chrono::steady_clock::now();
        int frames_in_window = 0;
        double fps = 0.0;

        while (true) {
            // 从会话获取最新帧（非阻塞）
            auto result = session.tryGetLatestFrame();

            if (result.has_value()) {
                ++frame_number;
                ++frames_in_window;

                // 计算FPS
                auto now = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration<double>(now - fps_start).count();
                if (elapsed >= 1.0) {
                    fps = frames_in_window / elapsed;
                    frames_in_window = 0;
                    fps_start = now;
                }

                // 叠加信息
                cv::Mat display = result->frame.clone();
                std::ostringstream overlay;
                overlay << "frame " << frame_number << "  "
                        << std::fixed << std::setprecision(1) << fps << " FPS";
                cv::putText(display, overlay.str(), cv::Point{20, 36},
                            cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar{40, 230, 90}, 2,
                            cv::LINE_AA);

                // 显示
                cv::imshow(window_name, display);
            }

            // 处理键盘事件
            int key = cv::waitKey(1) & 0xFF;

            // 检查窗口是否关闭
            double visibility = cv::getWindowProperty(window_name, cv::WND_PROP_VISIBLE);
            if (key == 'q' || key == 'Q' || visibility < 1.0) {
                break;
            }

            // 截图
            if ((key == 's' || key == 'S') && result.has_value()) {
                std::filesystem::create_directories("captures");
                std::filesystem::path filename;
                do {
                    ++capture_number;
                    filename = std::filesystem::path{"captures"} /
                               ("capture-" + std::to_string(capture_number) + ".png");
                } while (std::filesystem::exists(filename));

                if (cv::imwrite(filename.string(), result->frame)) {
                    std::cout << "Saved " << filename << std::endl;
                } else {
                    std::cerr << "Failed to save " << filename << std::endl;
                }
            }

            // 检查错误
            auto error = session.getLastError();
            if (error.has_value()) {
                std::cerr << "Warning: " << *error << std::endl;
            }
        }

        // 6. 清理
        session.stop();
        cv::destroyAllWindows();
        std::cout << "Session stopped" << std::endl;
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}