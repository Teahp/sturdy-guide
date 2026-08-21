#include "camera/camera_session.hpp"
#include "camera/opencv_camera.hpp"
#include "camera/preview_application.hpp"

#include <exception>
#include <iostream>
#include <memory>
#include <string_view>

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

    auto source =
        std::make_unique<sturdy_guide::camera::OpenCvCamera>(device, width,
                                                             height);
    auto session =
        std::make_unique<sturdy_guide::camera::CameraSession>(std::move(source));
    sturdy_guide::camera::PreviewApplication app{std::move(session),
                                                 "Sturdy Guide Camera"};
    return app.run();
  } catch (const std::exception& error) {
    std::cerr << "camera: " << error.what() << '\n';
    return 1;
  }
}
