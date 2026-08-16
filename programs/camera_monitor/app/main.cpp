// 应用入口：参数解析、依赖组装（真实摄像头 → 采集会话 → 预览应用）与主循环。
#include "camera/camera_session.h"
#include "camera/fake_frame_source.h"
#include "camera/opencv_camera.h"
#include "camera/preview_app.h"

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
  bool fake = false;  // 演示模式：用假帧源，无需摄像头
};

struct ParseResult {
  bool show_help = false;
  Options options;
};

ParseResult parse_args(const int argc, const char* const argv[]) {
  ParseResult result;
  for (int index = 1; index < argc; ++index) {
    const std::string_view argument{argv[index]};
    if (argument == "--help" || argument == "-h") {
      result.show_help = true;
      return result;
    }
    if (argument == "--fake") {
      result.options.fake = true;
      continue;
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
      result.options.device = value;
    } else if (argument == "--width") {
      result.options.width = value;
    } else {
      result.options.height = value;
    }
  }
  if (result.options.device < 0 || result.options.width <= 0 ||
      result.options.height <= 0) {
    throw std::invalid_argument(
        "device must be non-negative and dimensions must be positive");
  }
  return result;
}

}  // namespace

int main(const int argc, const char* const argv[]) {
  try {
    const ParseResult parsed = parse_args(argc, argv);
    if (parsed.show_help) {
      std::cout << "Usage: " << argv[0]
                << " [--device INDEX] [--width PIXELS] [--height PIXELS]\n"
                << "       [--fake]  use a synthetic frame source (no camera)\n"
                << "Keys: S saves a frame; Q exits.\n";
      return 0;
    }

    // 组装依赖：真实摄像头（或 --fake 假帧源）→ 采集会话 → 预览应用。
    std::unique_ptr<camera::FrameSource> source;
    if (parsed.options.fake) {
      camera::FakeFrameSource::Options fake_options;
      fake_options.width = parsed.options.width;
      fake_options.height = parsed.options.height;
      source = std::make_unique<camera::FakeFrameSource>(fake_options);
    } else {
      source = std::make_unique<camera::OpenCvCamera>(
          parsed.options.device, parsed.options.width, parsed.options.height);
    }
    camera::CameraSession session(std::move(source));
    session.start();

    camera::PreviewApplication app(session);
    return app.run("Sturdy Guide Camera");
  } catch (const std::exception& error) {
    std::cerr << "camera: " << error.what() << '\n';
    return 1;
  }
}
