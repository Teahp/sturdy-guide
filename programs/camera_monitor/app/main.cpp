#include "camera/camera_session.hpp"
#include "camera/opencv_camera.hpp"
#include "preview_application.hpp"

#include <exception>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

// 主线程职责：解析参数、组装依赖（摄像头 -> 采集会话 -> 预览 UI）并驱动主循环。
// 窗口循环、FPS 叠加、按键与截图封装在 PreviewApplication 中，本文件不接触摄像头或画面。
int main(const int argc, const char* const argv[]) {
  try {
    int device = 0;
    int width = 1280;
    int height = 720;

    for (int index = 1; index < argc; ++index) {
      const std::string_view argument{argv[index]};
      if (argument == "--help" || argument == "-h") {
        std::cout << "Usage: " << argv[0]
                  << " [--device INDEX] [--width PIXELS] [--height PIXELS]\n"
                  << "Keys: S saves a frame; Q exits.\n";
        return 0;
      }

      if (argument != "--device" && argument != "--width" &&
          argument != "--height") {
        throw std::invalid_argument("unknown option: " +
                                    std::string{argument});
      }
      if (index + 1 >= argc) {
        throw std::invalid_argument(std::string{argument} +
                                    " requires an integer value");
      }

      const int value = std::stoi(argv[++index]);
      if (argument == "--device") {
        device = value;
      } else if (argument == "--width") {
        width = value;
      } else {
        height = value;
      }
    }

    if (device < 0 || width <= 0 || height <= 0) {
      throw std::invalid_argument(
          "device must be non-negative and dimensions must be positive");
    }

    // 依赖组装：真插头（摄像头）-> 采集会话（后台帮手）-> 预览 UI（主线程）。
    auto source = std::make_unique<sturdy_guide::camera::OpenCvCamera>(
        device, width, height);
    sturdy_guide::camera::CameraSession session{std::move(source)};
    session.start();

    sturdy_guide::camera::PreviewApplication app{session,
                                                 "Sturdy Guide Camera"};
    const int exit_code = app.run();
    session.stop();  // 干净地停下后台帮手；run() 内已关闭窗口。
    return exit_code;
  } catch (const std::exception& error) {
    std::cerr << "camera: " << error.what() << '\n';
    return 1;
  }
}
