#include "camera/camera_session.h"
#include "preview_application.h"
#include "opencv_camera.h"
#include <cstdlib>
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

Options parseArguments(int argc, const char* const argv[]) {
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
        const auto options = parseArguments(argc, argv);

        // 组装依赖
        auto camera = std::make_unique<camera::OpenCvCamera>(
            options.device, options.width, options.height);
        auto session = std::make_unique<camera::CameraSession>(std::move(camera));
        camera::PreviewApplication app(std::move(session));

        return app.run();

    } catch (const std::exception& e) {
        std::cerr << "camera: " << e.what() << '\n';
        return 1;
    }
}
