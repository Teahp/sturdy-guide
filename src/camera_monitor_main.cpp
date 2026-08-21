#include "../include/CameraSession.hpp"
#include "../include/OpenCVCamera.hpp"
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <chrono>
#include <string>

int main(int argc, char* argv[]) {
    int device = 0;
    int width = 1280;
    int height = 720;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: " << argv[0]
                      << " [--device INDEX] [--width PIXELS] [--height PIXELS]\n"
                      << "Keys: S saves a frame; Q exits.\n";
            return 0;
        }
        if (arg == "--device" && i + 1 < argc) {
            device = std::stoi(argv[++i]);
        } else if (arg == "--width" && i + 1 < argc) {
            width = std::stoi(argv[++i]);
        } else if (arg == "--height" && i + 1 < argc) {
            height = std::stoi(argv[++i]);
        }
    }

    try {
        auto camera = std::make_unique<OpenCVCamera>(device);
        if (!camera->isOpened()) {
            std::cerr << "Failed to open camera device " << device << "\n";
            return 1;
        }

        CameraSession session(std::move(camera));
        session.start();

        const std::string windowName = "Sturdy Guide Camera (Refactored)";
        cv::namedWindow(windowName, cv::WINDOW_NORMAL);

        std::size_t frameNumber = 0;
        std::size_t captureNumber = 0;
        std::size_t framesInWindow = 0;
        double fps = 0.0;
        auto rateStart = std::chrono::steady_clock::now();

        while (true) {
            auto frameOpt = session.getLatestFrame();
            if (!frameOpt) {
                cv::waitKey(10);
                continue;
            }
            cv::Mat frame = *frameOpt;
            ++frameNumber;
            ++framesInWindow;

            auto now = std::chrono::steady_clock::now();
            auto elapsed = now - rateStart;
            if (elapsed >= std::chrono::seconds(1)) {
                double sec = std::chrono::duration<double>(elapsed).count();
                fps = framesInWindow / sec;
                framesInWindow = 0;
                rateStart = now;
            }

            std::ostringstream overlay;
            overlay << "frame " << frameNumber << "  " << std::fixed
                    << std::setprecision(1) << fps << " FPS";
            cv::putText(frame, overlay.str(), cv::Point(20, 36),
                        cv::FONT_HERSHEY_SIMPLEX, 0.8,
                        cv::Scalar(40, 230, 90), 2, cv::LINE_AA);
            cv::imshow(windowName, frame);

            int key = cv::waitKey(1) & 0xFF;
            double visibility = cv::getWindowProperty(windowName, cv::WND_PROP_VISIBLE);
            if (key == 'q' || key == 'Q' || (visibility >= 0.0 && visibility < 1.0)) {
                break;
            }

            if (key == 's' || key == 'S') {
                std::filesystem::create_directories("captures");
                std::filesystem::path filename;
                do {
                    ++captureNumber;
                    filename = std::filesystem::path("captures") /
                               ("capture-" + std::to_string(captureNumber) + ".png");
                } while (std::filesystem::exists(filename));
                if (!cv::imwrite(filename.string(), frame)) {
                    std::cerr << "Failed to save " << filename.string() << "\n";
                } else {
                    std::cout << "Saved " << filename << "\n";
                }
            }
        }

        session.stop();
        session.join();
        cv::destroyAllWindows();
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
