#include "camera/camera_session.hpp"
#include "camera/opencv_camera.hpp"
#include "camera/preview_application.hpp"

#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

// 入口只负责：解析参数、组装依赖、启动应用。
// 不再包含设备访问、帧率统计、窗口事件或文件保存逻辑。
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

    // 依赖组装：真实摄像头后端 -> 采集会话 -> 主线程应用。
    auto source =
        std::make_unique<camera::OpenCvCamera>(device, width, height);
    camera::CameraSession session{std::move(source)};
    camera::PreviewApplication app{session};
    return app.run();
  } catch (const std::exception& error) {
    std::cerr << "camera: " << error.what() << '\n';
    return 1;
  }
}
