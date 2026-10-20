#include "camera/opencv_camera.hpp"
#include "camera/preview_application.hpp"

#include <exception>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace {

struct CommandLine {
  int device = 0;
  int width = 1280;
  int height = 720;
};

void print_usage(const std::string_view program) {
  std::cout << "Usage: " << program
            << " [--device INDEX] [--width PIXELS] [--height PIXELS]\n"
            << "Keys: S saves a frame; Q exits.\n";
}

// 参数解析只把文本变成数字，既不接触设备，也不创建窗口。
CommandLine parse(const int argc, const char* const argv[]) {
  CommandLine options;

  for (int index = 1; index < argc; ++index) {
    const std::string_view argument{argv[index]};
    if (argument == "--help" || argument == "-h") {
      continue;
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
      options.device = value;
    } else if (argument == "--width") {
      options.width = value;
    } else {
      options.height = value;
    }
  }

  if (options.device < 0 || options.width <= 0 || options.height <= 0) {
    throw std::invalid_argument(
        "device must be non-negative and dimensions must be positive");
  }
  return options;
}

bool wants_help(const int argc, const char* const argv[]) {
  for (int index = 1; index < argc; ++index) {
    const std::string_view argument{argv[index]};
    if (argument == "--help" || argument == "-h") {
      return true;
    }
  }
  return false;
}

}  // namespace

int main(const int argc, const char* const argv[]) {
  if (wants_help(argc, argv)) {
    print_usage(argv[0]);
    return 0;
  }

  try {
    const CommandLine options = parse(argc, argv);

    // 组装：应用只认识 CameraSession，会话独占 FrameSource，
    // 真设备与测试假设备因此共用同一接口，业务代码里不需要"测试模式"分支。
    auto source = std::make_unique<camera::OpenCvCamera>(
        camera::OpenCvCamera::Settings{options.device, options.width,
                                       options.height});
    auto session =
        std::make_unique<camera::CameraSession>(std::move(source));

    camera::PreviewApplication application{std::move(session), {}};
    return application.run();
  } catch (const std::exception& error) {
    std::cerr << "camera: " << error.what() << '\n';
    return 1;
  }
}
