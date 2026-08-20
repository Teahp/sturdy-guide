#include "camera/camera_session.hpp"
#include "camera/open_cv_camera.hpp"
#include "camera/preview_application.hpp"

#include <exception>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

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

    // 声明顺序决定析构顺序：app 先析构，再 session（stop+join），最后 source。
    std::unique_ptr<sturdy_guide::camera::FrameSource> source =
        std::make_unique<sturdy_guide::camera::OpenCvCamera>(
            options.device, options.width, options.height);
    sturdy_guide::camera::CameraSession session{std::move(source)};
    session.start();

    sturdy_guide::camera::PreviewApplication app{"Sturdy Guide Camera",
                                                 session};
    app.run();
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "camera: " << error.what() << '\n';
    return 1;
  }
}
