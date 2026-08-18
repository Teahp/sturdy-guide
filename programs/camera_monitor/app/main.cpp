#include <iostream>
#include <memory>
#include "preview_application.h"
#include "opencv_camera.h"

int main(int argc, char** argv) {
    try {
        auto camera = std::make_unique<OpenCvCamera>(0, 640, 480);
        if (!camera->open()) {
            std::cerr << "Failed to open real camera, falling back to fake source...\n";
            return -1;
        }
        auto session = std::make_unique<CameraSession>(std::move(camera));
        PreviewApplication app(std::move(session));
        return app.run(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "Fatal: " << e.what() << std::endl;
        return -1;
    }
}
