#include "camera/camera_session.hpp"
#include "camera/opencv_camera.hpp"
#include "camera/preview_application.hpp"

#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

struct Options {
  int device{0};
  int width{1280};
  int height{720};
};

void printUsage(const char* program) {
  std::cout << "Usage: " << program
            << " [--device INDEX] [--width PIXELS] [--height PIXELS]\n"
            << "Keys: S saves a frame; Q exits.\n";
}

int readIntegerOption(const std::string_view option, const char* value) {
  try {
    return std::stoi(value);
  } catch (const std::exception&) {
    throw std::invalid_argument(std::string{option} +
                                " requires an integer value");
  }
}

Options parseOptions(const int argc, const char* const argv[]) {
  Options options;

  for (int index = 1; index < argc; ++index) {
    const std::string_view argument{argv[index]};
    if (argument == "--help" || argument == "-h") {
      printUsage(argv[0]);
      std::exit(0);
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

    const int value = readIntegerOption(argument, argv[++index]);
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

}  // namespace

int main(const int argc, const char* const argv[]) {
  try {
    const Options options = parseOptions(argc, argv);
    auto source = std::make_unique<camera::OpenCvCamera>(
        options.device, options.width, options.height);
    camera::CameraSession session{std::move(source)};
    camera::PreviewApplication application{session};
    application.run();
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "camera: " << error.what() << '\n';
    return 1;
  }
}
