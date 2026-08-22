#include "camera/camera_session.hpp"
#include "camera/opencv_camera.hpp"
#include "camera/preview_application.hpp"

#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

struct Options {
  int device = 0;
  int width = 1280;
  int height = 720;
};

// 参数解析与校验保持起始版本行为：--help 打印用法并正常退出。
// 返回 false 表示已请求帮助，应直接退出。
bool parse_options(int argc, const char* const argv[], Options& options) {
  for (int index = 1; index < argc; ++index) {
    const std::string_view argument{argv[index]};
    if (argument == "--help" || argument == "-h") {
      std::cout << "Usage: " << argv[0]
                << " [--device INDEX] [--width PIXELS] [--height PIXELS]\n"
                << "Keys: S saves a frame; Q exits.\n";
      return false;
    }
    if (argument != "--device" && argument != "--width" &&
        argument != "--height") {
      throw std::invalid_argument("unknown option: " + std::string{argument});
    }
    if (index + 1 >= argc) {
      throw std::invalid_argument(std::string{argument} +
                                  " requires an integer value");
    }
    const int value = std::stoi(argv[++index]);
    if (argument == "--device") {
      options.device = value;
    } else if (argument == "--width") {
      options.width = value;
    } else {
      options.height = value;
    }
  }
  return true;
}

}  // namespace

int main(const int argc, const char* const argv[]) {
  try {
    Options options;
    if (!parse_options(argc, argv, options)) return 0;

    if (options.device < 0 || options.width <= 0 || options.height <= 0) {
      throw std::invalid_argument(
          "device must be non-negative and dimensions must be positive");
    }

    auto source = std::make_unique<sturdy_guide::camera::OpenCvCamera>();
    sturdy_guide::camera::CameraSession session(std::move(source));
    sturdy_guide::camera::PreviewApplication app(session);

    session.start(options.device, options.width, options.height);
    const int exit_code = app.run();
    session.stop();
    return exit_code;
  } catch (const std::exception& error) {
    std::cerr << "camera: " << error.what() << '\n';
    return 1;
  }
}
