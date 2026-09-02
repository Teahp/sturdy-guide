#include "preview_application.hpp"

#include "camera/camera_session.hpp"
#include "camera/opencv_camera.hpp"

#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

// Parses --device/--width/--height (or --help). Throws std::invalid_argument
// on unknown options or malformed values, exactly like the original program.
struct Options {
  int device = 0;
  int width = 1280;
  int height = 720;
};

Options parse_options(int argc, char* argv[]) {
  Options options;
  for (int index = 1; index < argc; ++index) {
    const std::string_view argument{argv[index]};
    if (argument == "--help" || argument == "-h") {
      std::cout << "Usage: " << argv[0]
                << " [--device INDEX] [--width PIXELS] [--height PIXELS]\n"
                << "Keys: S saves a frame; Q exits.\n";
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

}  // namespace

int main(int argc, char* argv[]) {
  try {
    const Options options = parse_options(argc, argv);

    // Assemble the dependency graph: real device -> session -> UI.
    auto source = std::make_unique<camera::OpenCvCamera>(
        options.device, options.width, options.height);
    camera::CameraSession session{std::move(source)};
    session.start();

    camera::PreviewApplication application{session};
    return application.run();
  } catch (const std::exception& error) {
    std::cerr << "camera: " << error.what() << '\n';
    return 1;
  }
}
