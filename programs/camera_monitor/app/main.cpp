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
  bool show_help = false;
};

Options parse_options(const int argc, const char* const argv[]) {
  Options options;
  for (int index = 1; index < argc; ++index) {
    const std::string_view argument{argv[index]};
    if (argument == "--help" || argument == "-h") {
      options.show_help = true;
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

void print_usage(const char* const program) {
  std::cout << "Usage: " << program
            << " [--device INDEX] [--width PIXELS] [--height PIXELS]\n"
            << "Keys: S saves a frame; Q exits.\n";
}

}  // namespace

int main(const int argc, const char* const argv[]) {
  try {
    const Options options = parse_options(argc, argv);
    if (options.show_help) {
      print_usage(argv[0]);
      return 0;
    }

    auto source = std::make_unique<sturdy_guide::camera::OpenCvCamera>(
        options.device, options.width, options.height);//独立摄像头源，使用 OpenCV 的 VideoCapture 类从指定设备捕获视频帧，并设置分辨率为指定的宽度和高度。
    sturdy_guide::camera::CameraSession session{std::move(source)};//创建一个 CameraSession 对象，传入摄像头源。CameraSession 管理摄像头会话，包括启动、停止和获取最新帧。
    session.start();//start后，启动工作线程，开始从摄像头源读取帧，并将其存储在缓冲区中。
  
    sturdy_guide::camera::PreviewApplication application{session};//展示摄像头捕获的帧，并允许用户保存帧或退出应用程序。
    return application.run();//运行预览应用程序，显示摄像头捕获的帧，并允许用户保存帧或退出应用程序。返回值为整数，表示应用程序的退出状态。
  } catch (const std::exception& error) {
    std::cerr << "camera: " << error.what() << '\n';
    return 1;
  }
}
