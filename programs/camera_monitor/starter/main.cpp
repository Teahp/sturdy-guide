#include "camera_monitor/camera_session.hpp"
#include "camera_monitor/opencv_camera.hpp"

#include <filesystem>
#include <iostream>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <string>

namespace {

int parse_device(int argc, char** argv) {
    for (int i = 1; i + 1 < argc; ++i) {
        if (std::string(argv[i]) == "--device") {
            return std::stoi(argv[i + 1]);
        }
    }
    return 0;
}

void draw_status(cv::Mat& frame) {
    cv::putText(
        frame,
        "s: save | q: quit",
        cv::Point(16, 28),
        cv::FONT_HERSHEY_SIMPLEX,
        0.7,
        cv::Scalar(0, 255, 0),
        2);
}

}  // namespace

int main(int argc, char** argv) {
    const int device = parse_device(argc, argv);

    try {
        camera_monitor::OpenCvCamera source(device);
        camera_monitor::CameraSession session(source);
        session.start();

        const std::string window = "camera monitor";
        cv::namedWindow(window, cv::WINDOW_AUTOSIZE);

        cv::Mat latest;
        int saved_count = 0;
        for (;;) {
            cv::Mat frame;
            if (session.try_pop(frame)) {
                latest = std::move(frame);
            }

            if (!latest.empty()) {
                cv::Mat display = latest.clone();
                draw_status(display);
                cv::imshow(window, display);
            }

            const int key = cv::waitKey(1);
            if (key == 's' && !latest.empty()) {
                std::filesystem::create_directories("captures");
                const std::string path =
                    "captures/frame_" + std::to_string(saved_count++) + ".png";
                if (!cv::imwrite(path, latest)) {
                    std::cerr << "failed to write " << path << '\n';
                } else {
                    std::cout << "saved " << path << '\n';
                }
            }

            if (key == 'q') {
                break;
            }
        }

        session.stop();
        session.join();
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }

    return 0;
}
