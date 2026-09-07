#include "camera/open_cv_camera.hpp"
#include "camera/fake_frame_source.hpp"
#include "camera/camera_session.hpp"
#include "camera/preview_application.hpp"
#include <iostream>
#include <memory>
#include <string>
#include <stdexcept>
#include <opencv2/imgcodecs.hpp>

struct Options {
    int device_index = 0;
    int width = 640;
    int height = 480;
    bool use_fake = false;
    bool show_help = false;
    bool save_screenshot = false;   // 新增：只保存截图不显示窗口
};

Options parse_options(int argc, char* argv[]) {
    Options opts;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") opts.show_help = true;
        else if (arg == "--fake") opts.use_fake = true;
        else if (arg == "--save-screenshot") opts.save_screenshot = true;
        else if (arg == "--device" && i + 1 < argc) opts.device_index = std::stoi(argv[++i]);
        else if (arg == "--width" && i + 1 < argc) opts.width = std::stoi(argv[++i]);
        else if (arg == "--height" && i + 1 < argc) opts.height = std::stoi(argv[++i]);
        else throw std::invalid_argument("未知选项：" + arg);
    }
    return opts;
}

void print_usage(const char* prog) {
    std::cout << "用法: " << prog << " [选项]\n"
              << "  --device INDEX      摄像头编号 (默认0)\n"
              << "  --width PIXELS      宽度 (默认640)\n"
              << "  --height PIXELS     高度 (默认480)\n"
              << "  --fake              使用假相机（不依赖硬件）\n"
              << "  --save-screenshot   保存一帧截图到当前目录并退出（无需显示窗口）\n"
              << "  --help, -h          显示帮助\n";
}

int main(int argc, char* argv[]) {
    try {
        Options opts = parse_options(argc, argv);
        if (opts.show_help) { print_usage(argv[0]); return 0; }

        std::unique_ptr<sturdy_guide::camera::FrameSource> source;
        if (opts.use_fake || opts.save_screenshot) {   // 若是截图模式，强制使用假相机
            source = std::make_unique<sturdy_guide::camera::FakeFrameSource>(opts.width, opts.height);
            std::cout << "使用假相机（不连硬件）" << std::endl;
        } else {
            try {
                source = std::make_unique<sturdy_guide::camera::OpenCvCamera>(opts.device_index, opts.width, opts.height);
                std::cout << "成功打开真实摄像头 " << opts.device_index << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "警告：" << e.what() << "\n自动切换到假相机模式" << std::endl;
                source = std::make_unique<sturdy_guide::camera::FakeFrameSource>(opts.width, opts.height);
            }
        }

        // 如果是截图模式，取一帧保存后直接退出
        if (opts.save_screenshot) {
            auto frame = source->read();
            if (frame && !frame->empty()) {
                if (cv::imwrite("screenshot_fake.png", *frame)) {
                    std::cout << "截图已保存为 screenshot_fake.png" << std::endl;
                } else {
                    std::cerr << "截图保存失败" << std::endl;
                    return 1;
                }
            } else {
                std::cerr << "无法获取帧" << std::endl;
                return 1;
            }
            return 0;
        }

        // 正常模式：启动会话和窗口
        sturdy_guide::camera::CameraSession session(std::move(source));
        session.start();
        sturdy_guide::camera::PreviewApplication app(session);
        app.run();
        std::cout << "程序正常退出" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "错误：" << e.what() << std::endl;
        return 1;
    }
}
