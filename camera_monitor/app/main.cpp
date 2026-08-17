#include "camera/session.hpp"
#include "camera/opencv_camera.hpp"
#include "camera/fake_frame_source.hpp"
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>
#include <chrono>
#include <thread>
#include <cstdlib>
#include <getopt.h>

int main(int argc, char** argv) {
    int device = 0;
    int width = 640, height = 480;
    bool useFake = false;

    // 简单解析命令行参数
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--device" && i+1 < argc) {
            device = std::atoi(argv[++i]);
        } else if (arg == "--width" && i+1 < argc) {
            width = std::atoi(argv[++i]);
        } else if (arg == "--height" && i+1 < argc) {
            height = std::atoi(argv[++i]);
        } else if (arg == "--fake") {
            useFake = true;
        }
    }

    try {
        std::unique_ptr<camera::FrameSource> source;
        if (useFake) {
            source = std::make_unique<camera::FakeFrameSource>(width, height);
            std::cout << "Using fake frame source" << std::endl;
        } else {
            auto real = std::make_unique<camera::OpenCvCamera>(device);
            if (!real->isOpened()) {
                std::cerr << "Error: Cannot open camera device " << device << std::endl;
                return 1;
            }
            real->setProperty(cv::CAP_PROP_FRAME_WIDTH, width);
            real->setProperty(cv::CAP_PROP_FRAME_HEIGHT, height);
            source = std::move(real);
        }

        camera::CameraSession session(std::move(source));
        session.start();

        cv::namedWindow("Camera", cv::WINDOW_AUTOSIZE);

        bool exit = false;
        while (!exit) {
            camera::Frame frame;
            if (session.getLatestFrame(frame)) {
                // 计算 FPS
                static int64_t frameCount = 0;
                static double fps = 0.0;
                static int64_t lastTime = cv::getTickCount();
                frameCount++;
                int64_t currentTime = cv::getTickCount();
                double elapsed = (currentTime - lastTime) / cv::getTickFrequency();
                if (elapsed >= 1.0) {
                    fps = frameCount / elapsed;
                    frameCount = 0;
                    lastTime = currentTime;
                }
                // 在画面上叠加 FPS 和帧序号
                cv::Mat display = frame.image.clone();
                std::string fpsText = "FPS: " + std::to_string(static_cast<int>(fps));
                std::string frameText = "Frame: " + std::to_string(frame.frame_index);
                cv::putText(display, fpsText, cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 0), 2);
                cv::putText(display, frameText, cv::Point(10, 60), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 0), 2);
                cv::imshow("Camera", display);
            }

            int key = cv::waitKey(30);
            if (key == 'q' || key == 'Q' || key == 27) { // ESC
                exit = true;
            }
            if (key == 's' || key == 'S') {
                camera::Frame currentFrame;
                if (session.getLatestFrame(currentFrame)) {
                    std::string filename = "screenshot_" + std::to_string(currentFrame.frame_index) + ".png";
                    cv::imwrite(filename, currentFrame.image);
                    std::cout << "Saved screenshot: " << filename << std::endl;
                }
            }
            // 检查后台异常
            try {
                session.rethrowException();
            } catch (const std::exception& e) {
                std::cerr << "Background exception: " << e.what() << std::endl;
                exit = true;
            }
        }

        session.stop();
        cv::destroyAllWindows();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
